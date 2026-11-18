#include "QuantumCameraPresentation.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

void FQuantumTravelEnvelope::Reset()
{ Strength = Elapsed = Start = 0; Phase = EQuantumPresentationPhase::Idle; }
float FQuantumTravelEnvelope::TravelFov(float Baseline, float Amount)
{
    if (!FMath::IsFinite(Baseline) || !FMath::IsFinite(Amount)) return Baseline;
    return Baseline + FMath::Min(8.f, FMath::Max(0.f, 115.f - Baseline)) * FMath::Clamp(Amount, 0.f, 1.f);
}

void FQuantumTravelEnvelope::Update(float Seconds, EQuantumPresentationPhase NewPhase)
{
    if (Phase != NewPhase) { Start = Strength; Elapsed = 0; Phase = NewPhase; }
    if (!FMath::IsFinite(Seconds) || Seconds <= 0) return;
    Elapsed += Seconds;
    const bool Active = Phase == EQuantumPresentationPhase::Departure || Phase == EQuantumPresentationPhase::Loading;
    const float Progress = FMath::Clamp(Elapsed / (Active ? .9f : .8f), 0.f, 1.f);
    const float Ease = Progress * Progress * (3.f - 2.f * Progress);
    Strength = FMath::Lerp(Start, Active ? 1.f : 0.f, Ease);
}

bool UQuantumTravelCameraModifier::ModifyCamera(float, FMinimalViewInfo& POV)
{
    // Camera changes can occur between the UI timer and camera evaluation.
    // Never apply the old traveler's effect to a newly selected view target.
    if (!ExpectedViewTarget.IsValid() || GetViewTarget() != ExpectedViewTarget.Get()) return false;
    const float Amount = FMath::Clamp(Strength, 0.f, 1.f);
    // Smooth bounded additive FOV, evaluated from the fresh POV, not accumulated.
    POV.FOV = FQuantumTravelEnvelope::TravelFov(POV.FOV, Amount);
    return false; // Other camera modifiers remain active.
}

FQuantumCameraPresentation::~FQuantumCameraPresentation() { Reset(); }
void FQuantumCameraPresentation::Reset()
{
    if (auto* Owned = Modifier.Get())
    {
        Owned->Strength = 0;
        Owned->DisableModifier(true);
        if (auto* Camera = Manager.Get()) Camera->RemoveCameraModifier(Owned);
    }
    Modifier.Reset(); Manager.Reset(); ViewTarget.Reset(); Envelope.Reset(); Mode=INDEX_NONE;
}
void FQuantumCameraPresentation::Update(APlayerController* Controller, float Seconds, EQuantumPresentationPhase Phase, int32 CameraMode)
{
    auto* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
    auto* Target = Controller ? Controller->GetViewTarget() : nullptr;
    if (!Camera || !Target) { Reset(); return; }
    if (Modifier.IsValid() && (Manager.Get() != Camera || ViewTarget.Get() != Target || Mode!=CameraMode)) Reset();
    Mode=CameraMode;
    Envelope.Update(Seconds, Phase);
    if (!IsVisible())
    {
        if (Phase == EQuantumPresentationPhase::Idle || Phase == EQuantumPresentationPhase::Countdown) Reset();
        return;
    }
    if (!Modifier.IsValid())
    {
        Manager = Camera; ViewTarget = Target;
        Modifier = Cast<UQuantumTravelCameraModifier>(Camera->AddNewCameraModifier(UQuantumTravelCameraModifier::StaticClass()));
    }
    if (auto* Owned = Modifier.Get()) { Owned->ExpectedViewTarget = Target; Owned->Strength = Envelope.Strength; }
}
