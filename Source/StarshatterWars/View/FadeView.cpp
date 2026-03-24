/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         FadeView.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo
    Destroyer Studios LLC
    Copyright (C) 1997–2004.

    OVERVIEW
    ========
    FadeView implementation.

    Implements a legacy-style fade state machine using Unreal Engine
    timing utilities. FadeView computes a normalized fade scalar
    (0.0–1.0) over time and forwards it to the rendering layer via
    a callback sink.

    FadeView does not render directly.
*/

#include "FadeView.h"
#include "GameStructs.h"
#include "HAL/PlatformTime.h"
#include "Math/UnrealMathUtility.h"

// -------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------

double FadeView::NowMs()
{
    return FPlatformTime::Seconds() * 1000.0;
}

// -------------------------------------------------------------------
// Construction
// -------------------------------------------------------------------

FadeView::FadeView(
    View* InParent,
    double InFadeInSec,
    double InFadeOutSec,
    double InHoldSec
)
    : View(
        InParent ? InParent->GetScreen() : nullptr,
        0,
        0,
        InParent ? InParent->Width() : 0,
        InParent ? InParent->Height() : 0
    )
{
    FadeInMs = InFadeInSec * 1000.0;
    FadeOutMs = InFadeOutSec * 1000.0;
    HoldMs = InHoldSec * 1000.0;

    StepMs = 0.0;
    TimeMs = 0.0;
    bFast = true;

    State = EFadeState::StateStart;

    CurrentFade = (FadeInMs > 0.0) ? 0.0f : 1.0f;
    ApplyFade(CurrentFade);
}

// -------------------------------------------------------------------
// Internal
// -------------------------------------------------------------------

void FadeView::ApplyFade(float Fade01)
{
    Fade01 = FMath::Clamp(Fade01, 0.0f, 1.0f);
    CurrentFade = Fade01;

    if (FadeSink)
    {
        FadeSink(CurrentFade);
    }
}

// -------------------------------------------------------------------
// View override
// -------------------------------------------------------------------

void FadeView::Refresh()
{
    double DeltaMs = 0.0;

    if (State == EFadeState::StateStart)
    {
        TimeMs = NowMs();
    }
    else if (State != EFadeState::StateDone)
    {
        const double NewTime = NowMs();
        DeltaMs = NewTime - TimeMs;
        TimeMs = NewTime;
    }

    switch (State)
    {
    case EFadeState::StateStart:
        StepMs = 0.0;
        State = EFadeState::State2;
        break;

    case EFadeState::State2:
        if (FadeInMs > 0.0)
            ApplyFade(0.0f);

        StepMs = 0.0;
        State = EFadeState::StateIn;
        break;

    case EFadeState::StateIn:
        if (FadeInMs > 0.0)
        {
            if (StepMs < FadeInMs)
            {
                ApplyFade(static_cast<float>(StepMs / FadeInMs));
                StepMs += DeltaMs;
            }
            else
            {
                ApplyFade(1.0f);
                StepMs = 0.0;
                State = EFadeState::StateHold;
            }
        }
        else
        {
            ApplyFade(1.0f);
            StepMs = 0.0;
            State = EFadeState::StateHold;
        }
        break;

    case EFadeState::StateHold:
        if (StepMs < HoldMs)
        {
            StepMs += DeltaMs;
        }
        else
        {
            StepMs = 0.0;
            State = EFadeState::StateOut;
        }
        break;

    case EFadeState::StateOut:
        if (FadeOutMs > 0.0)
        {
            if (StepMs < FadeOutMs)
            {
                ApplyFade(1.0f - static_cast<float>(StepMs / FadeOutMs));
                StepMs += DeltaMs;
            }
            else
            {
                ApplyFade(0.0f);
                StepMs = 0.0;
                State = EFadeState::StateDone;
            }
        }
        else
        {
            ApplyFade(0.0f);
            StepMs = 0.0;
            State = EFadeState::StateDone;
        }
        break;

    case EFadeState::StateDone:
    default:
        break;
    }
}
