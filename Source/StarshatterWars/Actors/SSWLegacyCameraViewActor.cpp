#include "SSWLegacyCameraViewActor.h"

#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "Engine/World.h"

ASSWLegacyCameraViewActor::ASSWLegacyCameraViewActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASSWLegacyCameraViewActor::ActivateView(float BlendSeconds)
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->bAutoManageActiveCameraTarget = false;
            PC->SetViewTargetWithBlend(this, FMath::Max(0.0f, BlendSeconds));
        }
    }
}

void ASSWLegacyCameraViewActor::ApplyLegacyView(
    const FVector& WorldPosition,
    const FVector& WorldForward,
    const FVector& WorldRight,
    const FVector& WorldUp)
{
    if (WorldPosition.ContainsNaN() || WorldForward.ContainsNaN() ||
        WorldRight.ContainsNaN() || WorldUp.ContainsNaN())
        return;

    const FVector Forward = WorldForward.GetSafeNormal();
    const FVector Up = (WorldUp - Forward * FVector::DotProduct(WorldUp, Forward)).GetSafeNormal();
    if (Forward.IsNearlyZero() || Up.IsNearlyZero())
        return;

    // Same view-basis conversion as the existing ASSWCameraManager bridge.
    // Keep the (X,Y,Z)->(Z,X,Y) simulation axis mapping in the runtime.
    SetActorLocationAndRotation(WorldPosition,
        FRotationMatrix::MakeFromXZ(Forward, Up).Rotator());
}
