/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterKeyboardSettings.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterKeyboardSettings
    - UE-native keyboard settings container (config-backed).
    - Source of truth for keyboard behavior and key remapping.
    - SaveGame import is handled by UStarshatterKeyboardSubsystem.
    - Runtime apply is delegated to ApplyToRuntimeKeyboard().
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"

#include "GameStructs.h" // FStarshatterKeyboardConfig, EStarshatterInputAction
#include "StarshatterKeyboardSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class STARSHATTERWARS_API UStarshatterKeyboardSettings : public UObject
{
    GENERATED_BODY()

public:
    // Config-backed CDO accessor
    static UStarshatterKeyboardSettings* Get();

    // Reload from ini
    void Load();

    // Save to ini
    void Save() const;

    // Clamp values and clean mappings
    void Sanitize();

    // Runtime apply entry point (called by subsystem)
    void ApplyToRuntimeKeyboard(UObject* WorldContextObject);

public:
    const FStarshatterKeyboardConfig& GetKeyboardConfig() const { return Keyboard; }
    void SetKeyboardConfig(const FStarshatterKeyboardConfig& In);

private:
    // Enhanced Input hook (implementation detail, safely guarded)
    void ApplyToEnhancedInput(UObject* WorldContextObject);

private:
    // Stored in DefaultGame.ini and user config
    UPROPERTY(Config, EditAnywhere, Category = "Keyboard")
    FStarshatterKeyboardConfig Keyboard;
};
