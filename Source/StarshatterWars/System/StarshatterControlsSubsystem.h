/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterControlsSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterControlsSubsystem
    - GameInstance subsystem entry point for controls.
    - SaveGame pipeline:
        LoadFromSaveGame(SG) imports data into UStarshatterControlsSettings (config-backed CDO)
        ApplySettingsToRuntime(...) installs Enhanced Input mapping context for the selected model
    - Also includes compatibility wrappers for old call sites.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StarshatterControlsSubsystem.generated.h"

class UObject;
class UStarshatterSettingsSaveGame;

UCLASS()
class STARSHATTERWARS_API UStarshatterControlsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Convenience accessor
    static UStarshatterControlsSubsystem* Get(UObject* WorldContextObject);

    // UGameInstanceSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Preferred API
    void ApplySettingsToRuntime(UObject* WorldContextObject);

    // Compatibility no-arg wrapper (BootSubsystem currently calls this)
    UFUNCTION()
    void ApplySettingsToRuntime();

    // SaveGame import (BootSubsystem calls this)
    UFUNCTION()
    void LoadFromSaveGame(UStarshatterSettingsSaveGame* SaveGame);
};
