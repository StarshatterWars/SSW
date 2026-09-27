#include "SSWCameraManager.h"

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"

ASSWCameraManager::ASSWCameraManager()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
}

void
ASSWCameraManager::BeginPlay()
{
	Super::BeginPlay();
}

void
ASSWCameraManager::ActivateCamera(float BlendTime)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[SSWCameraManager] ActivateCamera failed: World is null"));

		return;
	}

	APlayerController* PC =
		World->GetFirstPlayerController();

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

	UE_LOG(LogTemp, Log,
		TEXT("[SSWCameraManager] ActivateCamera Camera=%s Loc=%s Rot=%s Blend=%.2f"),
		*GetName(),
		*GetActorLocation().ToString(),
		*GetActorRotation().ToString(),
		BlendTime);
}

// ----------------------------------------------------
// CAMERA MODES
// ----------------------------------------------------

void
ASSWCameraManager::SetBodyOrbitView(
	AActor* Target,
	const FVector& OrbitPoint)
{
	if (!Target)
	{
		return;
	}

	TargetActor = Target;

	Azimuth = OrbitPoint.X;
	Elevation = OrbitPoint.Y;

	Range = FMath::Clamp(
		(double)OrbitPoint.Z,
		(double)MinRange,
		(double)MaxRange);

	Mode = ESSWCameraMode::BodyOrbit;

	UpdateOrbit(0.0f);
}

void
ASSWCameraManager::SetOrbitRates(
	const FVector& Rates)
{
	AzRate = Rates.X;
	ElRate = Rates.Y;
	RangeRate = Rates.Z;
}

void
ASSWCameraManager::SetActorFollowView(
	AActor* Target,
	const FVector& Offset,
	const FRotator& Rotator)
{
	if (!Target)
	{
		ClearCamera();
		return;
	}

	TargetActor = Target;
	FollowOffset = Offset;
	FollowRotator = Rotator;

	Mode = ESSWCameraMode::ActorFollow;

	UpdateActorFollow(0.0f);
}

void
ASSWCameraManager::SetStaticView(
	const FVector& Location,
	const FRotator& Rotation)
{
	Mode = ESSWCameraMode::Static;

	SetActorLocation(Location);
	SetActorRotation(Rotation);
}

void
ASSWCameraManager::ClearCamera()
{
	Mode = ESSWCameraMode::None;
	TargetActor = nullptr;
}

// ----------------------------------------------------
// TICK
// ----------------------------------------------------

void
ASSWCameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float SafeDelta =
		FMath::Clamp(
			DeltaTime,
			0.0f,
			0.033f);

	switch (Mode)
	{
	case ESSWCameraMode::BodyOrbit:
		UpdateOrbit(SafeDelta);
		break;

	case ESSWCameraMode::ActorFollow:
		UpdateActorFollow(SafeDelta);
		break;

	case ESSWCameraMode::GroupFollow:
		UpdateGroupFollow(SafeDelta);
		break;

	default:
		break;
	}
}

// ----------------------------------------------------
// ORBIT
// ----------------------------------------------------

void
ASSWCameraManager::UpdateOrbit(float DeltaTime)
{
	if (!TargetActor)
	{
		return;
	}

	const double Seconds =
		FMath::Clamp(
			(double)DeltaTime,
			0.0,
			0.033);

	Azimuth += AzRate * Seconds;
	Elevation += ElRate * Seconds;
	Range *= (1.0 + RangeRate * Seconds);

	Range =
		FMath::Clamp(
			Range,
			(double)MinRange,
			(double)MaxRange);

	const FVector Target =
		TargetActor->GetActorLocation();

	const double Dx =
		Range * FMath::Sin(Azimuth) * FMath::Cos(Elevation);

	const double Dy =
		Range * FMath::Cos(Azimuth) * FMath::Cos(Elevation);

	const double Dz =
		Range * FMath::Sin(Elevation);

	const FVector DesiredCamLoc =
		Target + FVector(
			(float)Dx,
			(float)Dy,
			(float)Dz);

	const FVector CurrentCamLoc =
		GetActorLocation();

	const double JumpDistance =
		FVector::Dist(
			CurrentCamLoc,
			DesiredCamLoc);

	if (DeltaTime <= 0.0f ||
		JumpDistance > 10000.0)
	{
		SetActorLocation(DesiredCamLoc);
		LookAt(Target);
		return;
	}

	const FVector SmoothedCamLoc =
		FMath::VInterpTo(
			CurrentCamLoc,
			DesiredCamLoc,
			DeltaTime,
			12.0f);

	SetActorLocation(SmoothedCamLoc);

	const FRotator DesiredRot =
		(Target - SmoothedCamLoc).Rotation();

	const FRotator SmoothedRot =
		FMath::RInterpTo(
			GetActorRotation(),
			DesiredRot,
			DeltaTime,
			10.0f);

	SetActorRotation(SmoothedRot);
}

// ----------------------------------------------------
// ACTOR FOLLOW
// ----------------------------------------------------

void
ASSWCameraManager::UpdateActorFollow(float DeltaTime)
{
	if (!TargetActor)
		return;

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;

	TargetActor->GetActorBounds(
		true,
		BoundsOrigin,
		BoundsExtent,
		false);

	const FVector TargetLoc =
		BoundsExtent.IsNearlyZero()
		? TargetActor->GetActorLocation()
		: BoundsOrigin;

	const float Radius =
		FMath::Max(
			BoundsExtent.Size(),
			100.0f);

	const FVector ForwardDir =
		TargetActor->GetActorForwardVector().GetSafeNormal();

	const FVector RightDir =
		TargetActor->GetActorRightVector().GetSafeNormal();

	const FVector UpDir =
		TargetActor->GetActorUpVector().GetSafeNormal();

	FVector LocalOffset = FollowOffset;

	if (LocalOffset.IsNearlyZero())
	{
		LocalOffset = ComputeTightFollowOffset(TargetActor);
	}

	FVector DesiredOffset =
		ForwardDir * LocalOffset.X +
		RightDir * LocalOffset.Y +
		UpDir * LocalOffset.Z;

	if (DesiredOffset.IsNearlyZero())
	{
		DesiredOffset =
			-ForwardDir * Radius;
	}

	const float OffsetSize =
		DesiredOffset.Size();

	const float MinSafeDistance =
		Radius * 2.5f;

	if (OffsetSize < MinSafeDistance)
	{
		DesiredOffset =
			DesiredOffset.GetSafeNormal() * MinSafeDistance;
	}

	const FVector DesiredCamLoc =
		TargetLoc + DesiredOffset;

	const FVector CurrentCamLoc =
		GetActorLocation();

	const double JumpDistance =
		FVector::Dist(CurrentCamLoc, DesiredCamLoc);

	if (DeltaTime <= 0.0f || JumpDistance > Radius * 4.0f)
	{
		SetActorLocation(DesiredCamLoc);
		SetActorRotation((TargetLoc - DesiredCamLoc).Rotation());
		return;
	}

	const float SafeDelta =
		FMath::Clamp(DeltaTime, 0.0f, 0.033f);

	const FVector SmoothedCamLoc =
		FMath::VInterpTo(
			CurrentCamLoc,
			DesiredCamLoc,
			SafeDelta,
			12.0f);

	SetActorLocation(SmoothedCamLoc);

	const FRotator DesiredRot =
		(TargetLoc - SmoothedCamLoc).Rotation();

	const FRotator SmoothedRot =
		FMath::RInterpTo(
			GetActorRotation(),
			DesiredRot,
			SafeDelta,
			10.0f);

	SetActorRotation(SmoothedRot);
}

// ----------------------------------------------------
// LOOK AT
// ----------------------------------------------------

void
ASSWCameraManager::LookAt(const FVector& Target)
{
	const FVector Dir =
		Target - GetActorLocation();

	if (Dir.IsNearlyZero())
	{
		return;
	}

	SetActorRotation(
		Dir.Rotation());
}

// ----------------------------------------------------
// GROUP FOLLOW
// ----------------------------------------------------

void
ASSWCameraManager::SetGroupFollowView(
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

	if (GroupTargets.Num() <= 0)
	{
		ClearCamera();
		return;
	}

	GroupFollowOffset =
		Offset.IsNearlyZero()
		? FVector(-3000.0f, 1200.0f, 800.0f)
		: Offset;

	GroupVelocityDir =
		InVelocityDir.GetSafeNormal();

	if (GroupVelocityDir.IsNearlyZero())
	{
		GroupVelocityDir =
			FVector::ForwardVector;
	}

	GroupLookAhead =
		InLookAhead;

	Mode =
		ESSWCameraMode::GroupFollow;

	UpdateGroupFollow(0.0f);
}

void
ASSWCameraManager::UpdateGroupFollow(float DeltaTime)
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

	const FVector Center =
		GroupBox.GetCenter();

	const FVector Extent =
		GroupBox.GetExtent();

	FVector VelocityDir =
		AverageVelocity.GetSafeNormal();

	if (VelocityDir.IsNearlyZero())
	{
		VelocityDir = GroupVelocityDir;
	}

	if (VelocityDir.IsNearlyZero())
	{
		VelocityDir =
			ValidTargets[0]->GetActorForwardVector();
	}

	if (VelocityDir.IsNearlyZero())
	{
		VelocityDir =
			FVector::ForwardVector;
	}

	const float Radius =
		FMath::Max(
			Extent.Size(),
			800.0f);

	const float DistanceScale =
		FMath::Clamp(
			Radius / 800.0f,
			1.0f,
			3.0f);

	const FVector LocalOffset =
		GroupFollowOffset * DistanceScale;

	const FRotator MovementRot =
		VelocityDir.Rotation();

	const FVector DesiredCamLoc =
		Center +
		MovementRot.RotateVector(LocalOffset);

	const FVector CurrentCamLoc =
		GetActorLocation();

	const double JumpDistance =
		FVector::Dist(
			CurrentCamLoc,
			DesiredCamLoc);

	const FVector LookTarget =
		Center +
		(VelocityDir * GroupLookAhead);

	if (DeltaTime <= 0.0f ||
		JumpDistance > 15000.0)
	{
		SetActorLocation(DesiredCamLoc);

		SetActorRotation(
			(LookTarget - DesiredCamLoc).Rotation());

		return;
	}

	const float SafeDelta =
		FMath::Clamp(
			DeltaTime,
			0.0f,
			0.033f);

	const FVector SmoothedCamLoc =
		FMath::VInterpTo(
			CurrentCamLoc,
			DesiredCamLoc,
			SafeDelta,
			GroupLagSpeed);

	SetActorLocation(SmoothedCamLoc);

	const FRotator DesiredRot =
		(LookTarget - SmoothedCamLoc).Rotation();

	const FRotator SmoothedRot =
		FMath::RInterpTo(
			GetActorRotation(),
			DesiredRot,
			SafeDelta,
			GroupLagSpeed);

	SetActorRotation(SmoothedRot);
}

// ----------------------------------------------------
// TIGHT FOLLOW OFFSET
// ----------------------------------------------------

FVector
ASSWCameraManager::ComputeTightFollowOffset(AActor* Target) const
{
	if (!Target)
	{
		return FVector(-3000.0f, 0.0f, 1000.0f);
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;

	Target->GetActorBounds(
		true,
		Origin,
		Extent,
		false);

	const float Radius =
		FMath::Max3(
			Extent.X,
			Extent.Y,
			Extent.Z);

	if (Radius <= KINDA_SMALL_NUMBER)
	{
		return FVector(-3000.0f, 0.0f, 1000.0f);
	}

	const float Distance =
		Radius * 4.0f;

	const float Height =
		Radius * 1.25f;

	return FVector(
		-Distance,
		0.0f,
		Height);
}