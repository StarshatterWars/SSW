#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "SSWLegacyCameraViewActor.generated.h"

// Unreal presentation endpoint only. Does NOT own legacy CameraManager state,
// advance simulation, interpret DataTables, or integrate orbit/follow motion.
UCLASS()
class STARSHATTERWARS_API ASSWLegacyCameraViewActor : public ACameraActor
{
    GENERATED_BODY()

public:
    ASSWLegacyCameraViewActor();

    void ActivateView(float BlendSeconds = 0.0f);

    // Inputs are ALREADY converted to UE world position and UE basis axes.
    void ApplyLegacyView(const FVector& WorldPosition,
                         const FVector& WorldForward,
                         const FVector& WorldRight,
                         const FVector& WorldUp);
};
