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
	{
		return;
	}

	const FVector TargetLoc =
		TargetActor->GetActorLocation();

	const FVector ForwardDir =
		TargetActor->GetActorForwardVector().GetSafeNormal();

	const FVector RightDir =
		TargetActor->GetActorRightVector().GetSafeNormal();

	const FVector UpDir =
		TargetActor->GetActorUpVector().GetSafeNormal();

	FVector LocalOffset =
		FollowOffset;

	if (LocalOffset.IsNearlyZero())
	{
		LocalOffset = FVector(
			-2500.0f,
			900.0f,
			650.0f);
	}

	const FVector DesiredCamLoc =
		TargetLoc +
		ForwardDir * LocalOffset.X +
		RightDir * LocalOffset.Y +
		UpDir * LocalOffset.Z;

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

		SetActorRotation(
			(TargetLoc - DesiredCamLoc).Rotation());

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
ASSWCameraManager::ComputeTightFollowOffset(
	AActor* Target) const
{
	if (!Target)
	{
		return FVector(
			-600.0f,
			150.0f,
			200.0f);
	}

	FBox Bounds(ForceInit);

	TArray<UPrimitiveComponent*> PrimComps;

	Target->GetComponents<UPrimitiveComponent>(
		PrimComps);

	for (UPrimitiveComponent* Comp : PrimComps)
	{
		if (Comp && Comp->IsRegistered())
		{
			Bounds += Comp->Bounds.GetBox();
		}
	}

	if (!Bounds.IsValid)
	{
		return FVector(
			-600.0f,
			150.0f,
			200.0f);
	}

	const FVector Extent =
		Bounds.GetExtent();

	const float Radius =
		Extent.Size();

	const float Distance =
		FMath::Clamp(
			Radius * 1.6f,
			250.0f,
			2500.0f);

	return FVector(
		-Distance,
		Distance * 0.25f,
		Distance * 0.35f);
}