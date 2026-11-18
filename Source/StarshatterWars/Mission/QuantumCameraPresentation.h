#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "QuantumCameraPresentation.generated.h"

enum class EQuantumPresentationPhase : uint8 { Idle, Countdown, Departure, Loading, Arrival };

// Cosmetic clock only. Never feeds timing, warp factors or movement back into the sim.
struct FQuantumTravelEnvelope
{
    void Update(float Seconds, EQuantumPresentationPhase NewPhase);
    void Reset();
    static float TravelFov(float Baseline, float Amount);
    float Strength = 0;
    EQuantumPresentationPhase Phase = EQuantumPresentationPhase::Idle;
private:
    float Elapsed = 0, Start = 0;
};

UCLASS()
class UQuantumTravelCameraModifier : public UCameraModifier
{
    GENERATED_BODY()
public:
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& POV) override;
    float Strength = 0;
    TWeakObjectPtr<AActor> ExpectedViewTarget;
};

// A modifier acts on each freshly calculated POV. It never writes the camera's
// baseline FOV/postprocess settings, so removal restores the current camera mode.
struct FQuantumCameraPresentation
{
    ~FQuantumCameraPresentation();
    void Update(class APlayerController* Controller, float Seconds, EQuantumPresentationPhase Phase, int32 CameraMode = 0);
    void Reset();
    bool IsVisible() const { return Envelope.Strength > .001f; }
private:
    FQuantumTravelEnvelope Envelope;
    TWeakObjectPtr<class APlayerCameraManager> Manager;
    TWeakObjectPtr<AActor> ViewTarget;
    TWeakObjectPtr<UQuantumTravelCameraModifier> Modifier;
    int32 Mode = INDEX_NONE;
};
