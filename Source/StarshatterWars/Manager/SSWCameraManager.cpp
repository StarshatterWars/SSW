#include "SSWCameraManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

ASSWCameraManager::ASSWCameraManager()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>("Root");
    SetRootComponent(Root);

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(Root);
}

void ASSWCameraManager::BeginPlay()
{
    Super::BeginPlay();
}

void ASSWCameraManager::ActivateCamera(float BlendTime)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetViewTargetWithBlend(this, BlendTime);
    }
}

// ----------------------------------------------------
// CAMERA MODES
// ----------------------------------------------------

void ASSWCameraManager::SetBodyOrbitView(AActor* Target, const FVector& OrbitPoint)
{
    if (!Target) return;

    TargetActor = Target;

    Azimuth = OrbitPoint.X;
    Elevation = OrbitPoint.Y;
    Range = FMath::Clamp((double)OrbitPoint.Z, (double)MinRange, (double)MaxRange);

    Mode = ESSWCameraMode::BodyOrbit;

    UpdateOrbit(0.0f);
}

void ASSWCameraManager::SetOrbitRates(const FVector& Rates)
{
    AzRate = Rates.X;
    ElRate = Rates.Y;
    RangeRate = Rates.Z;
}

void ASSWCameraManager::SetActorFollowView(
    AActor* Target,
    const FVector& Offset,
    const FRotator& Rotator)
{
    TargetActor = Target;
    FollowOffset = Offset;
    FollowRotator = Rotator;

    Mode = ESSWCameraMode::ActorFollow;

    UpdateActorFollow(0.0f);
}

void ASSWCameraManager::SetStaticView(const FVector& Location, const FRotator& Rotation)
{
    Mode = ESSWCameraMode::Static;
    SetActorLocation(Location);
    SetActorRotation(Rotation);
}

void ASSWCameraManager::ClearCamera()
{
    Mode = ESSWCameraMode::None;
    TargetActor = nullptr;
}

// ----------------------------------------------------
// TICK
// ----------------------------------------------------

void ASSWCameraManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    switch (Mode)
    {
    case ESSWCameraMode::BodyOrbit:
        UpdateOrbit(DeltaTime);
        break;

    case ESSWCameraMode::ActorFollow:
        UpdateActorFollow(DeltaTime);
        break;

    default:
        break;
    }
}

// ----------------------------------------------------
// ORBIT (THIS IS THE LEGACY MAGIC)
// ----------------------------------------------------

void ASSWCameraManager::UpdateOrbit(float DeltaTime)
{
    if (!TargetActor)
    {
        return;
    }

    const double Seconds = FMath::Clamp((double)DeltaTime, 0.0, 0.2);

    Azimuth += AzRate * Seconds;
    Elevation += ElRate * Seconds;
    Range *= (1.0 + RangeRate * Seconds);

    Range = FMath::Clamp(Range, (double)MinRange, (double)MaxRange);

    const FVector Target = TargetActor->GetActorLocation();

    const double Dx = Range * FMath::Sin(Azimuth) * FMath::Cos(Elevation);
    const double Dy = Range * FMath::Cos(Azimuth) * FMath::Cos(Elevation);
    const double Dz = Range * FMath::Sin(Elevation);

    // Unreal axis mapping:
    // X = horizontal
    // Y = depth
    // Z = vertical
    const FVector CamLoc = Target + FVector((float)Dx, (float)Dy, (float)Dz);

    SetActorLocation(CamLoc);
    LookAt(Target);
}

// ----------------------------------------------------
// ACTOR FOLLOW
// ----------------------------------------------------

void ASSWCameraManager::UpdateActorFollow(float DeltaTime)
{
    if (!TargetActor) return;

    FVector Target = TargetActor->GetActorLocation();

    FVector Offset = FollowOffset;

    if (Offset.IsNearlyZero())
    {
        float Size = TargetActor->GetComponentsBoundingBox().GetExtent().Size();
        Offset = FVector(0, 0, -Size * 4.0f);
    }

    FVector Rotated = FollowRotator.RotateVector(Offset);

    FVector CamLoc = Target + Rotated;

    SetActorLocation(CamLoc);
    LookAt(Target);
}

// ----------------------------------------------------
// LOOK AT
// ----------------------------------------------------

void ASSWCameraManager::LookAt(const FVector& Target)
{
    FVector Dir = Target - GetActorLocation();
    SetActorRotation(Dir.Rotation());
}