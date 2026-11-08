#include "MissionViewCamera.h"
AMissionViewCamera::AMissionViewCamera(const FObjectInitializer& Initializer)
    : Super(Initializer.SetDefaultSubobjectClass<UMissionViewCameraComponent>(TEXT("CameraComponent"))) {}
void UMissionViewCameraComponent::GetCameraView(float DeltaTime,FMinimalViewInfo& DesiredView)
{
    Super::GetCameraView(DeltaTime,DesiredView);
    DesiredView.PerspectiveNearClipPlane=bCockpitView?.05f:-1.f;
    if (bCockpitView)
    {
        DesiredView.PostProcessSettings.bOverride_MotionBlurAmount = true;
        DesiredView.PostProcessSettings.MotionBlurAmount = 0.0f;
        DesiredView.PostProcessBlendWeight = 1.0f;
    }
}
