/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterControlsSettings.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterControlsSettings.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

// Enhanced Input:
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

UStarshatterControlsSettings* UStarshatterControlsSettings::Get()
{
    return GetMutableDefault<UStarshatterControlsSettings>();
}

void UStarshatterControlsSettings::Load()
{
    ReloadConfig();
    Sanitize();
}

void UStarshatterControlsSettings::Save() const
{
    const_cast<UStarshatterControlsSettings*>(this)->SaveConfig();
}

void UStarshatterControlsSettings::Sanitize()
{
    switch (Controls.ControlModel)
    {
    case EStarshatterControlModel::Arcade:
    case EStarshatterControlModel::FlightSim:
    case EStarshatterControlModel::Hybrid:
        break;
    default:
        Controls.ControlModel = EStarshatterControlModel::FlightSim;
        break;
    }

    Controls.JoystickIndex = FMath::Max(0, Controls.JoystickIndex);
    Controls.ThrottleAxis = FMath::Max(0, Controls.ThrottleAxis);
    Controls.RudderAxis = FMath::Max(0, Controls.RudderAxis);
    Controls.JoystickSensitivity = FMath::Clamp(Controls.JoystickSensitivity, 0, 10);

    Controls.MouseSensitivity = FMath::Clamp(Controls.MouseSensitivity, 0, 50);
    Controls.bMouseInvert = Controls.bMouseInvert ? true : false;

    MappingPriority = FMath::Clamp(MappingPriority, 0, 10);
}

void UStarshatterControlsSettings::SetControlModelEnum(EStarshatterControlModel In)
{
    Controls.ControlModel = In;
}

void UStarshatterControlsSettings::SetJoystickIndex(int32 V)
{
    Controls.JoystickIndex = FMath::Max(0, V);
}

void UStarshatterControlsSettings::SetThrottleAxis(int32 V)
{
    Controls.ThrottleAxis = FMath::Max(0, V);
}

void UStarshatterControlsSettings::SetRudderAxis(int32 V)
{
    Controls.RudderAxis = FMath::Max(0, V);
}

void UStarshatterControlsSettings::SetJoystickSensitivity(int32 V)
{
    Controls.JoystickSensitivity = FMath::Clamp(V, 0, 10);
}

void UStarshatterControlsSettings::SetMouseSensitivity(int32 V)
{
    Controls.MouseSensitivity = FMath::Clamp(V, 0, 50);
}

void UStarshatterControlsSettings::SetMouseInvert(bool bV)
{
    Controls.bMouseInvert = bV;
}

void UStarshatterControlsSettings::ApplyToRuntimeControls(UObject* WorldContextObject)
{
    Load();
    ApplyToEnhancedInput(WorldContextObject);
}

UInputMappingContext* UStarshatterControlsSettings::ResolveContextForModel() const
{
    TSoftObjectPtr<UInputMappingContext> Soft;

    switch (Controls.ControlModel)
    {
    case EStarshatterControlModel::Arcade:
        Soft = ArcadeContext;
        break;
    case EStarshatterControlModel::FlightSim:
        Soft = FlightSimContext;
        break;
    case EStarshatterControlModel::Hybrid:
        Soft = HybridContext;
        break;
    default:
        Soft = FlightSimContext;
        break;
    }

    if (!Soft.IsNull())
    {
        return Soft.LoadSynchronous();
    }

    return nullptr;
}

void UStarshatterControlsSettings::InstallOnlySelectedContext(UEnhancedInputLocalPlayerSubsystem* Subsystem, UInputMappingContext* Selected) const
{
    if (!Subsystem)
        return;

    // Remove known contexts (ignore null).
    if (!ArcadeContext.IsNull())
    {
        UInputMappingContext* Ctx = ArcadeContext.Get();
        if (!Ctx) Ctx = ArcadeContext.LoadSynchronous();
        if (Ctx) Subsystem->RemoveMappingContext(Ctx);
    }

    if (!FlightSimContext.IsNull())
    {
        UInputMappingContext* Ctx = FlightSimContext.Get();
        if (!Ctx) Ctx = FlightSimContext.LoadSynchronous();
        if (Ctx) Subsystem->RemoveMappingContext(Ctx);
    }

    if (!HybridContext.IsNull())
    {
        UInputMappingContext* Ctx = HybridContext.Get();
        if (!Ctx) Ctx = HybridContext.LoadSynchronous();
        if (Ctx) Subsystem->RemoveMappingContext(Ctx);
    }

    if (Selected)
    {
        Subsystem->AddMappingContext(Selected, MappingPriority);
    }

    Subsystem->RequestRebuildControlMappings();
}

void UStarshatterControlsSettings::ApplyToEnhancedInput(UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return;

    ULocalPlayer* LP = World->GetFirstLocalPlayerFromController();
    if (!LP)
        return;

    UEnhancedInputLocalPlayerSubsystem* EIS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!EIS)
        return;

    UInputMappingContext* Selected = ResolveContextForModel();
    InstallOnlySelectedContext(EIS, Selected);

    // Sensitivity and invert:
    // Enhanced Input has no global mouse sensitivity or invert settings.
    // Apply these in your input handling code (pawn/controller) by reading:
    // UStarshatterControlsSettings::Get()->GetControlsConfig().
}
