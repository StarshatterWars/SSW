// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "CentralSunActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "GalaxyManager.h"
#include "StarUtils.h"
#include "SystemMapUtils.h"

ACentralSunActor::ACentralSunActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Root Component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Sun Mesh
	SunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SunMesh"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		SunMesh->SetStaticMesh(SphereMesh.Object);
	}

	SunMesh->SetupAttachment(RootScene);
	SunMesh->SetRelativeScale3D(FVector(3.0f));
	SunMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	SunMesh->SetCastShadow(false);	
	
	// Scene Capture
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->SetRelativeLocation(FVector(-600.f, 0.f, 0.f));
	SceneCapture->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	SceneCapture->FOVAngle = 60.f;
	
	// Use final color with alpha
	//SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	// HDR capture matches your RTF_RGBA16f render target and preserves emissive
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

	// Optional: keep PP off if you want “raw” emissive; turn on if you want bloom/tonemap in the thumbnail.
	// SceneCapture->ShowFlags.SetPostProcessing(true);
	SceneCapture->ShowFlags.SetPostProcessing(false);

	// Flags to avoid black sky/background
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetSkyLighting(false);
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetMotionBlur(false);
	SceneCapture->ShowFlags.SetScreenSpaceReflections(false);
	SceneCapture->ShowFlags.SetDirectionalLights(true);
	SceneCapture->ShowFlags.SetPostProcessing(false);	
	SceneCapture->ShowFlags.SetMaterials(true);
	SceneCapture->ShowOnlyActors.Empty(); // optional
	SceneCapture->ShowOnlyComponents.Add(SunMesh);
	SceneCapture->bCaptureEveryFrame = false;      // recommended for manual capture
	SceneCapture->bCaptureOnMovement = true;       // helps with refresh on transform change
}

ACentralSunActor* ACentralSunActor::SpawnWithSpectralClass(
	UWorld* World,
	const FVector& Location,
	const FRotator& Rotation,
	TSubclassOf<ACentralSunActor> ActorClass,
	ESPECTRAL_CLASS InSpectralClass,
	float InRadius,
	FString InName)
{
	if (!World || !*ActorClass) return nullptr;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Defer construction to allow property assignment before BeginPlay
	Params.bDeferConstruction = true;

	// Spawn actor, but defer BeginPlay
	ACentralSunActor* NewActor = World->SpawnActor<ACentralSunActor>(ActorClass, Location, Rotation, Params);
	if (!NewActor) return nullptr;

	// Set SpectralClass before BeginPlay
	NewActor->SpectralClass = InSpectralClass;
	NewActor->Radius = InRadius;
	NewActor->StarName = InName;

	NewActor->FinishSpawning(FTransform(Rotation, Location));
	NewActor->ApplyStarVisuals(NewActor->SpectralClass);	

	return NewActor;
}

void ACentralSunActor::BeginPlay()
{
	Super::BeginPlay();
	CurrentRotation = GetActorRotation();
}

void ACentralSunActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//AddActorLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
}

void ACentralSunActor::ApplyStarVisuals(ESPECTRAL_CLASS Class)
{
	// Create the render target
	SunRenderTarget = SystemMapUtils::EnsureRenderTarget(
		this,
		StarName,
		StarUtils::GetRenderTargetResolutionForRadius(Radius),
		SceneCapture,
		SunMesh
	);

	SceneCapture->TextureTarget = SunRenderTarget;
	
	// Texture
	UE_LOG(LogTemp, Warning, TEXT("ACentralSunActor::ApplyStarVisuals(): SpectralClass = %s, StarColor = R=%.2f G=%.2f B=%.2f"),
		*UEnum::GetValueAsString(Class),
		StarColor.R, StarColor.G, StarColor.B);

	// Use StarUtils for consistent visuals
	StarColor = StarUtils::GetColor(Class);
	float GlowStrength = StarUtils::GetGlowStrength(Class);

	const float EmissiveStrength = StarUtils::GetEmissiveFromClass(Class);
	//const float EmissiveStrength = GlowStrength;

	float SunspotStrength = StarUtils::GetSunspotStrength(Class);

	// Create dynamic material
	UMaterialInstanceDynamic* DynMat = SystemMapUtils::CreatePreviewMID(
		this,
		StarBaseMaterial,
		StarTexture,
		StarName
	);

	// Apply to mesh
	SunMesh->SetMaterial(0, DynMat);
	StarMaterialInstance = DynMat;

	// Capture scene (now safe!)
	SunMesh->MarkRenderStateDirty();
	SceneCapture->CaptureScene();

	StarMaterialInstance->SetVectorParameterValue("StarColor", StarColor);
	//StarMaterialInstance->SetVectorParameterValue("Main Color High", StarColor);
	//StarMaterialInstance->SetVectorParameterValue("Main Color Low", StarColor);
	StarMaterialInstance->SetScalarParameterValue("GlowStrength", GlowStrength/10);
	//StarMaterialInstance->SetTextureParameterValue("Sunspots", SunspotTexture);
	//StarMaterialInstance->SetScalarParameterValue("SunspotStrength", SunspotStrength);

	// Delay CaptureScene by one tick (safe on all platforms)
	SystemMapUtils::ScheduleSafeCapture(this, SceneCapture);
}

