#pragma once
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "MissionViewCamera.generated.h"

// The small imported cockpit needs a per-view near plane, not a global renderer change.
UCLASS()
class UMissionViewCameraComponent : public UCameraComponent
{
    GENERATED_BODY()
public:
    bool bCockpitView = false;
    virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
};
UCLASS()
class AMissionViewCamera : public ACameraActor
{
    GENERATED_BODY()
public:
    AMissionViewCamera(const FObjectInitializer& Initializer);
};
