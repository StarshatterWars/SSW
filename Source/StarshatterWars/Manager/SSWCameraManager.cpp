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
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SSWCameraManager] ActivateCamera failed: World is null"));
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SSWCameraManager] ActivateCamera failed: PlayerController is null"));
        return;
    }

    PC->bAutoManageActiveCameraTarget = false;

    PC->SetViewTargetWithBlend(
        this,
        FMath::Max(0.0f, BlendTime));

    PC->SetControlRotation(GetActorRotation());

    AActor* ViewTarget = PC->GetViewTarget();

    UE_LOG(LogTemp, Error,
        TEXT("[SSWCameraManager] ActivateCamera Camera=%s Loc=%s Rot=%s Blend=%.2f ViewTarget=%s"),
        *GetName(),
        *GetActorLocation().ToString(),
        *GetActorRotation().ToString(),
        BlendTime,
        *GetNameSafe(ViewTarget));
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

    case ESSWCameraMode::GroupFollow:
        UpdateGroupFollow(DeltaTime);
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
    if (!TargetActor)
    {
        return;
    }

    //--------------------------------------------------
    // TARGET + VELOCITY
    //--------------------------------------------------

    const FVector TargetLoc = TargetActor->GetActorLocation();

    FVector VelocityDir = TargetActor->GetVelocity().GetSafeNormal();

    if (VelocityDir.IsNearlyZero())
    {
        VelocityDir = TargetActor->GetActorForwardVector();
    }

    //--------------------------------------------------
    // OFFSET (CAMERA POSITION RELATIVE TO SHIP)
    //--------------------------------------------------

    FVector LocalOffset = FollowOffset;

    if (LocalOffset.IsNearlyZero())
    {
        LocalOffset = FVector(-2500.0f, 900.0f, 650.0f);
    }

    const FRotator MovementRot = VelocityDir.Rotation();

    const FVector DesiredCamLoc =
        TargetLoc + MovementRot.RotateVector(LocalOffset);

    //--------------------------------------------------
    // POSITION DAMPING
    //--------------------------------------------------

    const float LagSpeed = 5.0f;

    const FVector SmoothedCamLoc = FMath::VInterpTo(
        GetActorLocation(),
        DesiredCamLoc,
        DeltaTime,
        LagSpeed);

    SetActorLocation(SmoothedCamLoc);

    //--------------------------------------------------
    // LOOK TARGET (CENTERED ON SHIP)
    //--------------------------------------------------

    const float LookAheadAmount = 0.0f; // keep ship centered

    const FVector LookTarget =
        TargetLoc + (VelocityDir * LookAheadAmount);

    const FRotator DesiredRot =
        (LookTarget - SmoothedCamLoc).Rotation();

    //--------------------------------------------------
    // ROTATION DAMPING
    //--------------------------------------------------

    const FRotator SmoothedRot = FMath::RInterpTo(
        GetActorRotation(),
        DesiredRot,
        DeltaTime,
        LagSpeed);

    SetActorRotation(SmoothedRot);

    //--------------------------------------------------
    // DEBUG
    //--------------------------------------------------

    UE_LOG(LogTemp, Warning,
        TEXT("[SSWCameraManager] ActorFollow CENTERED Target=%s TargetLoc=%s Cam=%s VelDir=%s"),
        *GetNameSafe(TargetActor),
        *TargetLoc.ToString(),
        *SmoothedCamLoc.ToString(),
        *VelocityDir.ToString());
}

// ----------------------------------------------------
// LOOK AT
// ----------------------------------------------------

void ASSWCameraManager::LookAt(const FVector& Target)
{
    FVector Dir = Target - GetActorLocation();
    SetActorRotation(Dir.Rotation());
}

void ASSWCameraManager::SetGroupFollowView(
    const TArray<AActor*>& InTargets,
    const FVector& Offset,
    const FVector& InVelocityDir,
    float InLookAhead)
{
    GroupTargets.Empty();

    for (AActor* Actor : InTargets)
    {
        if (Actor)
        {
            GroupTargets.Add(Actor);
        }
    }

    GroupFollowOffset = Offset.IsNearlyZero()
        ? FVector(-3000.0f, 1200.0f, 800.0f)
        : Offset;

    GroupVelocityDir = InVelocityDir.GetSafeNormal();

    if (GroupVelocityDir.IsNearlyZero())
    {
        GroupVelocityDir = FVector::ForwardVector;
    }

    GroupLookAhead = InLookAhead;
    Mode = ESSWCameraMode::GroupFollow;

    UpdateGroupFollow(0.0f);
}

void ASSWCameraManager::UpdateGroupFollow(float DeltaTime)
{
    TArray<AActor*> ValidTargets;

    for (AActor* Actor : GroupTargets)
    {
        if (Actor)
        {
            ValidTargets.Add(Actor);
        }
    }

    if (ValidTargets.Num() <= 0)
    {
        return;
    }

    FBox GroupBox(ForceInit);
    FVector AverageVelocity = FVector::ZeroVector;

    for (AActor* Actor : ValidTargets)
    {
        GroupBox += Actor->GetActorLocation();
        AverageVelocity += Actor->GetVelocity();
    }

    const FVector Center = GroupBox.GetCenter();
    const FVector Extent = GroupBox.GetExtent();

    FVector VelocityDir = AverageVelocity.GetSafeNormal();

    if (VelocityDir.IsNearlyZero())
    {
        VelocityDir = GroupVelocityDir;
    }

    if (VelocityDir.IsNearlyZero())
    {
        VelocityDir = ValidTargets[0]->GetActorForwardVector();
    }

    if (VelocityDir.IsNearlyZero())
    {
        VelocityDir = FVector::ForwardVector;
    }

    const float Radius = FMath::Max(Extent.Size(), 800.0f);
    const float DistanceScale = FMath::Clamp(Radius / 800.0f, 1.0f, 3.0f);

    const FVector LocalOffset = GroupFollowOffset * DistanceScale;
    const FRotator MovementRot = VelocityDir.Rotation();

    const FVector DesiredCamLoc =
        Center + MovementRot.RotateVector(LocalOffset);

    const float SafeDelta = FMath::Clamp(DeltaTime, 0.0f, 0.1f);

    const FVector SmoothedCamLoc = FMath::VInterpTo(
        GetActorLocation(),
        DesiredCamLoc,
        SafeDelta,
        GroupLagSpeed);

    SetActorLocation(SmoothedCamLoc);

    const FVector LookTarget =
        Center + (VelocityDir * GroupLookAhead);

    const FRotator DesiredRot =
        (LookTarget - SmoothedCamLoc).Rotation();

    const FRotator SmoothedRot = FMath::RInterpTo(
        GetActorRotation(),
        DesiredRot,
        SafeDelta,
        GroupLagSpeed);

    SetActorRotation(SmoothedRot);

    UE_LOG(LogTemp, Warning,
        TEXT("[SSWCameraManager] GroupFollow Count=%d Center=%s Radius=%.2f VelDir=%s Cam=%s"),
        ValidTargets.Num(),
        *Center.ToString(),
        Radius,
        *VelocityDir.ToString(),
        *SmoothedCamLoc.ToString());
}

FVector ASSWCameraManager::ComputeTightFollowOffset(AActor* Target) const
{
    if (!Target)
    {
        return FVector(-600.f, 150.f, 200.f);
    }

    FBox Bounds(ForceInit);

    TArray<UPrimitiveComponent*> PrimComps;
    Target->GetComponents<UPrimitiveComponent>(PrimComps);

    for (UPrimitiveComponent* Comp : PrimComps)
    {
        if (Comp && Comp->IsRegistered())
        {
            Bounds += Comp->Bounds.GetBox();
        }
    }

    const FVector Extent = Bounds.GetExtent();
    const float Radius = Extent.Size();

    const float Distance = FMath::Clamp(Radius * 1.6f, 250.f, 2500.f);

    return FVector(
        -Distance,
        Distance * 0.25f,
        Distance * 0.35f
    );
}