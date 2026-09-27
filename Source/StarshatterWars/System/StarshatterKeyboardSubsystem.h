/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterKeyboardSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterKeyboardSubsystem
    - GameInstanceSubsystem that owns keyboard settings lifecycle.
    - Loads from SaveGame (UStarshatterSettingsSaveGame::KeyboardConfig).
    - Writes into config-backed UStarshatterKeyboardSettings (CDO) and saves to ini.
    - Applies runtime input mapping via Enhanced Input if available.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "StarshatterKeyboardSubsystem.generated.h"

class UStarshatterSettingsSaveGame;

UCLASS()
class STARSHATTERWARS_API UStarshatterKeyboardSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Convenience accessor (mirrors your other subsystems)
    static UStarshatterKeyboardSubsystem* Get(UObject* WorldContextObject);

    // SaveGame import
    void LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame);

    // Runtime apply (BootSubsystem calls this)
    void ApplySettingsToRuntime(UObject* WorldContextObject);

    // Optional helper: push current config-backed settings back into SaveGame
    void SaveToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const;
};
