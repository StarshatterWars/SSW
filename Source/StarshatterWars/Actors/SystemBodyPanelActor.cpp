#include "SystemBodyPanelActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "SystemMapUtils.h"

ASystemBodyPanelActor::ASystemBodyPanelActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootScene);
	BodyMesh->SetRelativeScale3D(FVector(3.0f));
	BodyMesh->SetRelativeLocation(FVector::ZeroVector);
	BodyMesh->SetCastShadow(false);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(SphereMesh.Object);
	}

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	const FBoxSphereBounds Bounds = BodyMesh->Bounds;
	const float R = FMath::Max(Bounds.SphereRadius, 10.f);

	// Put camera far enough back to see whole sphere reliably
	const float Distance = R * 3.0f;

	SceneCapture->SetRelativeLocation(FVector(-Distance, 0.f, 0.f));
	SceneCapture->SetRelativeRotation(FRotator::ZeroRotator);

	SceneCapture->SetupAttachment(RootScene);
	//SceneCapture->SetRelativeLocation(FVector(-600.f, 0.f, 0.f));
	//SceneCapture->SetRelativeRotation(FRotator::ZeroRotator);
	SceneCapture->FOVAngle = 60.f;
	//SceneCapture->FOVAngle = 30.f;
	//SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR; 
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = true;

	// Strip expensive flags for UI thumbnails
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetSkyLighting(false);
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetMotionBlur(false);
	SceneCapture->ShowFlags.SetScreenSpaceReflections(false);
	SceneCapture->ShowFlags.SetPostProcessing(false);

	RotationSpeed = 20.f;
	bApplyUIScale = false;
	bApplyAxisTilt = false;
}

void ASystemBodyPanelActor::BeginPlay()
{
	Super::BeginPlay();
	// If epoch was notxexplicitly set, anchor it to world time
		if (OrbitEpochSeconds <= 0.0)
		{
			OrbitEpochSeconds = GetWorld()->GetTimeSeconds();
		}

	// If no explicit seed was provided, fall back to actor name
	if (SystemSeed.IsEmpty())
	{
		SystemSeed = GetName();
	}

	InitializeBody();
}

void ASystemBodyPanelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ===================== ORBIT (WORLD LOCATION) =====================
	if (bEnableOrbit)
	{
		// Use deterministic orbit time (prefer authority time; falls back internally as needed)
		const double NowSeconds =
			OrbitAuthority ? OrbitAuthority->GetOrbitTimeSeconds()
			: GetOrbitTimeSeconds();

		UpdateOrbitLocation(NowSeconds);
	}

	// ===================== SPIN (MESH ROTATION) =====================
	// Spin is purely visual; choose ONE of the two time sources below:

	// Option A (recommended): spin uses world runtime (doesn't jump on load, looks smooth)
	const float SpinTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Option B: spin follows universe time (time-warp affects spin; can jump after load)
	// const float SpinTime = (float)GetOrbitTimeSeconds();

	const FRotator SpinRot = ComputeSpinRotation(SpinTime);

	if (bApplyAxisTilt)
	{
		const FRotator TiltRot(0.f, 0.f, GetBodyTiltDegrees());
		const FQuat FinalQ = FQuat(SpinRot) * FQuat(TiltRot);
		BodyMesh->SetRelativeRotation(FinalQ.Rotator());
	}
	else
	{
		BodyMesh->SetRelativeRotation(SpinRot);
	}
}	

void ASystemBodyPanelActor::InitBody(FString AssetType)
{
	// Texture (derived decides how)
	BodyTexture = LoadBodyTexture(AssetType);

	// Material
	BodyMaterialInstance = SystemMapUtils::CreatePreviewMID(
		this,
		GetBaseMaterial(),
		BodyTexture,
		GetBodyName()
	);

	if (BodyMaterialInstance)
	{
		BodyMesh->SetMaterial(0, BodyMaterialInstance);
	}

	// Render target
	const int32 Resolution = ComputeRenderTargetResolution(GetBodyRadius());
	UTextureRenderTarget2D* RT = SystemMapUtils::EnsureRenderTarget(
		this,
		GetBodyName(),
		Resolution,
		SceneCapture,
		BodyMesh
	);

	BodyRenderTarget = RT;
	SceneCapture->ShowOnlyComponents.Empty();
	SceneCapture->ShowOnlyComponents.Add(BodyMesh);
	SceneCapture->TextureTarget = BodyRenderTarget;

	// Capture now + safe capture next tick
	BodyMesh->MarkRenderStateDirty();

	SystemMapUtils::ScheduleSafeCapture(this, SceneCapture);

	UE_LOG(LogTemp, Warning, TEXT("InitBody() %s -> Mat: %s, Tex: %s, RT: %s"),
		*GetBodyName(),
		*GetNameSafe(BodyMaterialInstance),
		*GetNameSafe(BodyTexture),
		*GetNameSafe(BodyRenderTarget));
}

float ASystemBodyPanelActor::ComputeUIScale(float Radius) const
{
	return SystemMapUtils::GetBodyUIScale(Radius);
}

void ASystemBodyPanelActor::InitializeBody()
{
	const float Radius = GetBodyRadius();

	if (bApplyUIScale)
	{
		const float ScaleFactor = ComputeUIScale(Radius);
		BodyMesh->SetRelativeScale3D(FVector(ScaleFactor));
	}

	// Tilt is applied in Tick composition when bApplyAxisTilt = true.
	// If you want one-time tilt only (no tick composition), set bApplyAxisTilt=false
	// and apply tilt here explicitly in derived.
}

FRotator ASystemBodyPanelActor::ComputeSpinRotation(float WorldTimeSeconds) const
{
	// Preserve your existing behavior:
	return SystemMapUtils::GetBodyRotation(WorldTimeSeconds, RotationSpeed, BodyTilt);
}

UTexture2D* ASystemBodyPanelActor::LoadBodyTexture(FString AssetType)
{
	return SystemMapUtils::LoadBodyAssetTexture(AssetType, TextureName);
}

int32 ASystemBodyPanelActor::ComputeRenderTargetResolution(float Radius) const
{
	return FMath::Clamp(SystemMapUtils::GetRenderTargetResolutionForRadius(Radius), 256, 1024);
}

void ASystemBodyPanelActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
}

double ASystemBodyPanelActor::GetOrbitTimeSeconds() const
{
	const UTimerSubsystem* Timer = GetGameInstance()->GetSubsystem<UTimerSubsystem>();
	if (!Timer)
	{
		return 0.0;
	}

	const double UniverseNow = (double)Timer->GetUniverseTimeSeconds();
	return UniverseNow;
}

float ASystemBodyPanelActor::HashTo01(const FString& Key)
{
	const uint32 H = FCrc::StrCrc32(*Key);
	return (H & 0x00FFFFFFu) / float(0x01000000u);
}

void ASystemBodyPanelActor::InitializeOrbit(ASystemBodyPanelActor* InAuthority, float InOrbitRadiusKm, const FString& InOrbitSeed)
{
	OrbitAuthority = InAuthority;
	OrbitRadiusKm = InOrbitRadiusKm;
	OrbitSeed = InOrbitSeed;

	// Only enable if radius is valid
	bEnableOrbit = (OrbitRadiusKm > 0.f);

	// If seed notxprovided, fall back to actor name (still deterministic)
	if (OrbitSeed.IsEmpty())
	{
		OrbitSeed = GetName();
	}
}

FVector ASystemBodyPanelActor::GetOrbitCenterForThisBody() const
{
	// Moons can orbit moving planets by updating CustomOrbitCenter externally each tick
	if (bUseCustomOrbitCenter)
	{
		return CustomOrbitCenter;
	}

	// Default: orbit around authority's location (usually the star)
	if (OrbitAuthority)
	{
		return OrbitAuthority->GetOrbitCenter();
	}

	// Fallback if no authority (orbit around world origin)
	return FVector::ZeroVector;
}

void ASystemBodyPanelActor::UpdateOrbitLocation(double NowSeconds)
{
	if (!bEnableOrbit || OrbitRadiusKm <= 0.f)
	{
		return;
	}

	const FVector Center = GetOrbitCenterForThisBody();

	// Epoch comes from authority if set, otherwise local epoch
	const double Epoch = OrbitAuthority ? OrbitAuthority->OrbitEpochSeconds : OrbitEpochSeconds;

	// km -> cm (UE units)
	const float r = FMath::Max(OrbitRadiusKm * 100000.f, 1.f);

	// Deterministic starting phase from seed
	const float Phase0 = HashTo01(OrbitSeed) * 2.f * PI;

	// Simple Kepler-ish scaling: v -> 1/sqrt(r)
	const float RefR = FMath::Max(ReferenceRadiusKm * 100000.f, 1.f);
	const float v = BaseSpeedCmPerSec * FMath::Sqrt(RefR / r);
	const float Omega = v / r;

	const float Phase = Phase0 + Omega * float(NowSeconds - Epoch);

	const FVector NewLocation =
		Center + FVector(FMath::Cos(Phase) * r, FMath::Sin(Phase) * r, 0.f);

	SetActorLocation(NewLocation);
}