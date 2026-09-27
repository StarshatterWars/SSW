/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         NavLightComponent.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Implementation of NavLightComponent.

    This component never ticks itself. The owning ship
    calls AdvanceLight() each frame.
*/

#include "NavLightComponent.h"

UNavLightComponent::UNavLightComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetMobility(EComponentMobility::Movable);
    SetUseInverseSquaredFalloff(false);
    SetCastShadows(false);

    IntensityMultiplier = 1.0f;
    RadiusMultiplier = 1.0f;
    TimeAccumulator = 0.0f;
    bRenderedVisible = true;

    SetVisibility(true);
    SetHiddenInGame(false);
}

void UNavLightComponent::ApplyDefinition(const FShipNavLightDef& InDef)
{
    LightDef = InDef;

    SetRelativeLocation(LightDef.LocalOffset);
    SetRelativeRotation(LightDef.LocalRotation);

    ResetRuntime();
    ApplyVisualSettings();
    ApplyRenderedVisibility(true);

    UE_LOG(LogTemp, Warning,
        TEXT("[NavLightComponent] ApplyDefinition: %s Loc=%s Color=%s Intensity=%.1f Radius=%.1f Mode=%d BlinkInterval=%.2f Phase=%.2f"),
        *GetName(),
        *LightDef.LocalOffset.ToString(),
        *LightDef.Color.ToString(),
        LightDef.Intensity,
        LightDef.Radius,
        (int32)LightDef.Mode,
        LightDef.BlinkInterval,
        LightDef.PhaseOffset);
}

void UNavLightComponent::SetGlobalMultipliers(float InIntensityMultiplier, float InRadiusMultiplier)
{
    IntensityMultiplier = InIntensityMultiplier;
    RadiusMultiplier = InRadiusMultiplier;
    ApplyVisualSettings();
}

void UNavLightComponent::AdvanceLight(float DeltaTime, bool bSequenceActive)
{
    bool bShouldBeVisible = true;

    switch (LightDef.Mode)
    {
    case EShipNavLightMode::Steady:
        bShouldBeVisible = true;
        break;

    case EShipNavLightMode::Blink:
    {
        const float Interval = FMath::Max(0.05f, LightDef.BlinkInterval);
        TimeAccumulator += DeltaTime;

        const float TimeWithPhase = TimeAccumulator + LightDef.PhaseOffset;
        const float Wrapped = FMath::Fmod(TimeWithPhase, Interval);

        bShouldBeVisible = (Wrapped < (Interval * 0.5f));
        break;
    }

    case EShipNavLightMode::Sequence:
        bShouldBeVisible = bSequenceActive;
        break;

    default:
        bShouldBeVisible = true;
        break;
    }

    ApplyRenderedVisibility(bShouldBeVisible);
}

void UNavLightComponent::ResetRuntime()
{
    TimeAccumulator = 0.0f;
    bRenderedVisible = true;
}

void UNavLightComponent::ForceVisible(bool bInVisible)
{
    ApplyRenderedVisibility(bInVisible);
}

void UNavLightComponent::ApplyVisualSettings()
{
    SetLightColor(LightDef.Color);
    SetIntensity(LightDef.Intensity * IntensityMultiplier);
    SetAttenuationRadius(LightDef.Radius * RadiusMultiplier);

    /*
     * Keep these small. The ship scale may already be large.
     */
    SetSourceRadius(2.0f);
    SetSoftSourceRadius(4.0f);
}

void UNavLightComponent::ApplyRenderedVisibility(bool bInVisible)
{
    if (bRenderedVisible == bInVisible)
    {
        return;
    }

    bRenderedVisible = bInVisible;

    SetVisibility(bInVisible);
    SetHiddenInGame(!bInVisible);
}

void UNavLightComponent::SetRenderedFromLegacy(bool bInVisible)
{
    ApplyVisualSettings();
    ApplyRenderedVisibility(bInVisible);
}