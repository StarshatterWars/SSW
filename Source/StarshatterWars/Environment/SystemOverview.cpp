/*  Project nGenEx
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    SSW
	FILE:         SystemOverview.cpp
	AUTHOR:       Carlos Bott
*/

#include "SystemOverview.h"

#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"

#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

ASystemOverview::ASystemOverview()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(Root);
	KeyLight->SetIntensity(25000.f);
	KeyLight->SetAttenuationRadius(200000.f);
	KeyLight->SetRelativeLocation(FVector(-5000.f, 0.f, 5000.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = SphereFinder.Object;

	ConfigureCaptureForUI();
}

void ASystemOverview::BeginPlay()
{
	Super::BeginPlay();
}

void ASystemOverview::ConfigureCaptureForUI()
{
	if (!Capture) return;

	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->ProjectionType = ECameraProjectionMode::Perspective;
	Capture->FOVAngle = 30.f;

	Capture->bOverride_CustomNearClippingPlane = true;
	Capture->CustomNearClippingPlane = 1.0f;

	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetSkyLighting(false);
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetPostProcessing(false);

	// Uncomment for flat unlit icons:
	// Capture->ShowFlags.SetLighting(false);
}

void ASystemOverview::ClearBodies()
{
	for (UStaticMeshComponent* Comp : BodyMeshes)
	{
		if (!Comp) continue;
		Comp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Comp->UnregisterComponent();
		Comp->DestroyComponent();
	}
	BodyMeshes.Reset();
}

float ASystemOverview::ComputeBodyUniformScale(const FOverviewBody& Body) const
{
	const float RadiusUU = FMath::Max(Body.RadiusKm * KmToUU, 1.f);
	const float Scale = RadiusUU / SphereRadiusUU;
	return FMath::Clamp(Scale, MinBodyScale, MaxBodyScale);
}

FVector ASystemOverview::ComputeBodyWorldPosition(
	const FOverviewBody& Body,
	const FVector& ParentWorldPos
) const
{
	const float OrbitUU = Body.OrbitKm * KmToUU;

	const float A = FMath::DegreesToRadians(
		FMath::Fmod(Body.OrbitAngleDeg, 360.f)
	);
	const float Incl = FMath::DegreesToRadians(Body.InclinationDeg);

	const float X = FMath::Cos(A) * OrbitUU;
	const float Y = FMath::Sin(A) * OrbitUU;
	const float Z = FMath::Sin(Incl) * OrbitUU * 0.25f;

	return ParentWorldPos + FVector(X, Y, Z);
}

void ASystemOverview::BuildDiorama(const TArray<FOverviewBody>& InBodies)
{
	ClearBodies();

	if (!SphereMesh || InBodies.Num() == 0)
		return;

	BodyMeshes.Reserve(InBodies.Num());

	TArray<FVector> Positions;
	Positions.SetNum(InBodies.Num());

	for (int32 i = 0; i < InBodies.Num(); ++i)
	{
		const FOverviewBody& Body = InBodies[i];

		const FVector ParentPos =
			(Body.ParentIndex >= 0 && InBodies.IsValidIndex(Body.ParentIndex))
			? Positions[Body.ParentIndex]
			: FVector::ZeroVector;

		const FVector Pos =
			(i == 0 && Body.ParentIndex == INDEX_NONE)
			? FVector::ZeroVector
			: ComputeBodyWorldPosition(Body, ParentPos);

		Positions[i] = Pos;

		UStaticMeshComponent* Mesh =
			NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);

		Mesh->SetupAttachment(Root);
		AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();

		Mesh->SetStaticMesh(SphereMesh);
		Mesh->SetRelativeLocation(Pos);

		const float Scale = ComputeBodyUniformScale(Body);
		Mesh->SetRelativeScale3D(FVector(Scale));

		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);

		Mesh->UpdateBounds();
		Mesh->MarkRenderStateDirty();

		BodyMeshes.Add(Mesh);
	}

	FrameDiorama();
}

void ASystemOverview::FrameDiorama(float PaddingMultiplier)
{
	if (!Capture || BodyMeshes.Num() == 0)
		return;

	FBox Bounds(EForceInit::ForceInit);
	for (UStaticMeshComponent* Mesh : BodyMeshes)
	{
		if (Mesh)
			Bounds += Mesh->Bounds.GetBox();
	}

	const FVector Center = Bounds.GetCenter();
	const float Radius = Bounds.GetExtent().Size();

	const float FOVRad = FMath::DegreesToRadians(Capture->FOVAngle);
	const float Dist =
		(Radius * PaddingMultiplier) /
		FMath::Tan(FOVRad * 0.5f);

	const FVector CamDir(-1.f, 0.f, 0.f);
	const FVector CamPos = Center - CamDir * Dist;

	Capture->SetWorldLocation(CamPos);
	Capture->SetWorldRotation((Center - CamPos).Rotation());
}

void ASystemOverview::SetRenderTarget(UTextureRenderTarget2D* InRT)
{
	if (!Capture) return;

	Capture->TextureTarget = InRT;

	if (InRT)
	{
		InRT->ClearColor = FLinearColor::Black;
		InRT->UpdateResourceImmediate(true);
	}
}

void ASystemOverview::CaptureOnce()
{
	if (!Capture || !Capture->TextureTarget)
		return;

	if (UWorld* World = GetWorld())
	{
		FTimerHandle Temp;
		World->GetTimerManager().SetTimer(
			Temp,
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (Capture)
					{
						Capture->CaptureScene();
					}
				}),
			0.0f,
			false
		);
	}
	else
	{
		Capture->CaptureScene();
	}
}
