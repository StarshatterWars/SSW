/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterKeyboardSettings.cpp
    AUTHOR:       Carlos Bott
*/

#include "StarshatterKeyboardSettings.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Logging/LogMacros.h"

#if __has_include("EnhancedInputSubsystems.h")
#include "EnhancedInputSubsystems.h"
#define SSW_HAS_ENHANCED_INPUT 1
#else
#define SSW_HAS_ENHANCED_INPUT 0
#endif

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterKeyboardSettings, Log, All);

// Keep this for naming consistency with your UI and any future binding tables.
static FName InputActionToMappingName(EStarshatterInputAction Action)
{
    switch (Action)
    {
        // Core
    case EStarshatterInputAction::ExitGame:       return FName("ExitGame");
    case EStarshatterInputAction::Pause:          return FName("Pause");

        // Time
    case EStarshatterInputAction::TimeCompress:   return FName("TimeCompress");
    case EStarshatterInputAction::TimeExpand:     return FName("TimeExpand");
    case EStarshatterInputAction::TimeSkip:       return FName("TimeSkip");

        // Flight
    case EStarshatterInputAction::ThrottleUp:     return FName("ThrottleUp");
    case EStarshatterInputAction::ThrottleDown:   return FName("ThrottleDown");
    case EStarshatterInputAction::ThrottleZero:   return FName("ThrottleZero");
    case EStarshatterInputAction::ThrottleFull:   return FName("ThrottleFull");

        // Weapons
    case EStarshatterInputAction::CyclePrimary:   return FName("CyclePrimary");
    case EStarshatterInputAction::CycleSecondary: return FName("CycleSecondary");
    case EStarshatterInputAction::FirePrimary:    return FName("FirePrimary");
    case EStarshatterInputAction::FireSecondary:  return FName("FireSecondary");

        // Targeting
    case EStarshatterInputAction::LockTarget:     return FName("LockTarget");
    case EStarshatterInputAction::LockThreat:     return FName("LockThreat");
    case EStarshatterInputAction::TargetNext:     return FName("TargetNext");
    case EStarshatterInputAction::TargetPrevious: return FName("TargetPrevious");

        // Camera
    case EStarshatterInputAction::CameraNextView: return FName("CameraNextView");
    case EStarshatterInputAction::CameraChase:    return FName("CameraChase");
    case EStarshatterInputAction::CameraExternal: return FName("CameraExternal");
    case EStarshatterInputAction::CameraZoomIn:   return FName("CameraZoomIn");
    case EStarshatterInputAction::CameraZoomOut:  return FName("CameraZoomOut");

        // UI
    case EStarshatterInputAction::NavDialog:      return FName("NavDialog");
    case EStarshatterInputAction::WeaponDialog:   return FName("WeaponDialog");
    case EStarshatterInputAction::FlightDialog:   return FName("FlightDialog");
    case EStarshatterInputAction::EngineDialog:   return FName("EngineDialog");

        // Comms
    case EStarshatterInputAction::RadioMenu:      return FName("RadioMenu");
    case EStarshatterInputAction::CommandMode:    return FName("CommandMode");

        // Debug
    case EStarshatterInputAction::IncStardate:    return FName("IncStardate");
    case EStarshatterInputAction::DecStardate:    return FName("DecStardate");
    default:                                      break;
    }

    return NAME_None;
}

// ------------------------------------------------------------
// Static accessor
// ------------------------------------------------------------

UStarshatterKeyboardSettings* UStarshatterKeyboardSettings::Get()
{
    return GetMutableDefault<UStarshatterKeyboardSettings>();
}

// ------------------------------------------------------------
// Config lifecycle
// ------------------------------------------------------------

void UStarshatterKeyboardSettings::Load()
{
    ReloadConfig();
    Sanitize();
}

void UStarshatterKeyboardSettings::Save() const
{
    const_cast<UStarshatterKeyboardSettings*>(this)->SaveConfig();
}

void UStarshatterKeyboardSettings::SetKeyboardConfig(const FStarshatterKeyboardConfig& In)
{
    Keyboard = In;
    Sanitize();
}

// ------------------------------------------------------------
// Validation
// ------------------------------------------------------------

void UStarshatterKeyboardSettings::Sanitize()
{
    Keyboard.KeyRepeatDelaySeconds = FMath::Clamp(Keyboard.KeyRepeatDelaySeconds, 0.0f, 1.0f);
    Keyboard.KeyRepeatRateSeconds = FMath::Clamp(Keyboard.KeyRepeatRateSeconds, 0.01f, 0.50f);

    // Remove invalid keys (enum key itself is always present, so only validate the FKey)
    for (auto It = Keyboard.RemappedKeys.CreateIterator(); It; ++It)
    {
        const FKey& Key = It.Value();
        if (!Key.IsValid())
        {
            It.RemoveCurrent();
        }
    }
}

// ------------------------------------------------------------
// Runtime apply
// ------------------------------------------------------------

void UStarshatterKeyboardSettings::ApplyToRuntimeKeyboard(UObject* WorldContextObject)
{
    Load();
    ApplyToEnhancedInput(WorldContextObject);
}

void UStarshatterKeyboardSettings::ApplyToEnhancedInput(UObject* WorldContextObject)
{
#if !SSW_HAS_ENHANCED_INPUT
    UE_LOG(LogStarshatterKeyboardSettings, Warning, TEXT("Enhanced Input headers not available. Skipping keyboard apply."));
    return;
#else
    if (!WorldContextObject)
        return;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
        return;

    // NOTE: Boot can run before a LocalPlayer exists.
    ULocalPlayer* LP = World->GetFirstLocalPlayerFromController();
    if (!LP)
    {
        UE_LOG(LogStarshatterKeyboardSettings, Verbose, TEXT("No LocalPlayer yet. Keyboard apply deferred."));
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSS = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!InputSS)
    {
        UE_LOG(LogStarshatterKeyboardSettings, Warning, TEXT("Enhanced Input subsystem not found on LocalPlayer."));
        return;
    }

    if (!Keyboard.bKeyboardEnabled)
    {
        UE_LOG(LogStarshatterKeyboardSettings, Log, TEXT("Keyboard disabled by settings."));
        return;
    }

    // Architecture hook:
    // - Boot-time: ensure mappings rebuild.
    // - UI-time: when you implement per-action remap, you will apply Keyboard.RemappedKeys into
    //   either a runtime mapping context or Enhanced Input user settings.
    UE_LOG(LogStarshatterKeyboardSettings, Log, TEXT("Keyboard settings applied (Remaps=%d)."), Keyboard.RemappedKeys.Num());

    InputSS->RequestRebuildControlMappings();
#endif
}
