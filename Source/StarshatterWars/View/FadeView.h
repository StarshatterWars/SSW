/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         FadeView.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo
    Destroyer Studios LLC
    Copyright (C) 1997–2004.

    OVERVIEW
    ========
    FadeView
    --------
    Legacy-style fade controller view.

    FadeView is a non-rendering UI view responsible for managing
    timed fade-in, hold, and fade-out transitions. It computes
    a normalized fade scalar (0.0–1.0) and emits that value to
    the rendering layer via a callback sink.

    - Plain C++ (NOT a UObject)
    - Uses Unreal timing (FPlatformTime)
    - Rendering is delegated to Video/Screen/Widget layers
    - State machine uses shared EFadeState (GameStructs.h)

    Typical usage:
        - Splash screens
        - Scene transitions
        - Cinematic fades
*/

#pragma once

#include "CoreMinimal.h"
#include <functional>

#include "GameStructs.h"   // <-- EFadeState now lives here
#include "View.h"

class FadeView : public View
{
public:
    static const char* TYPENAME() { return "FadeView"; }

public:
    FadeView(
        View* InParent,
        double InFadeInSec = 1.0,
        double InFadeOutSec = 1.0,
        double InHoldSec = 4.0
    );

    virtual ~FadeView() override = default;

    // Operations:
    virtual void Refresh() override;

    bool Done() const { return State == EFadeState::StateDone; }
    bool Holding() const { return State == EFadeState::StateHold; }

    // Control:
    void FastFade(int InFast) { bFast = (InFast != 0); }
    void FadeIn(double InSec) { FadeInMs = InSec * 1000.0; }
    void FadeOut(double InSec) { FadeOutMs = InSec * 1000.0; }
    void StopHold() { HoldMs = 0.0; }

    // Current fade scalar (0..1)
    float GetFade() const { return CurrentFade; }

    // Renderer hook
    void SetFadeSink(std::function<void(float)> InSink)
    {
        FadeSink = std::move(InSink);
    }

private:
    static double NowMs();
    void ApplyFade(float Fade01);

private:
    double FadeInMs = 1000.0;
    double FadeOutMs = 1000.0;
    double HoldMs = 4000.0;

    double StepMs = 0.0;
    double TimeMs = 0.0;

    bool bFast = true;

    // NOTE: now using shared enum from GameStructs.h
    EFadeState State = EFadeState::StateStart;

    float CurrentFade = 1.0f;

    std::function<void(float)> FadeSink;
};
