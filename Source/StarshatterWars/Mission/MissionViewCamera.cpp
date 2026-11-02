#include "MissionViewCamera.h"
AMissionViewCamera::AMissionViewCamera(const FObjectInitializer& Initializer)
    : Super(Initializer.SetDefaultSubobjectClass<UMissionViewCameraComponent>(TEXT("CameraComponent"))) {}
void UMissionViewCameraComponent::GetCameraView(float DeltaTime,FMinimalViewInfo& DesiredView)
{
    Super::GetCameraView(DeltaTime,DesiredView);
    DesiredView.PerspectiveNearClipPlane=bCockpitView?.05f:-1.f;
}
