/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterAudioSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterAudioSubsystem
    - GameInstanceSubsystem wrapper around UStarshatterAudioSettings (config-backed CDO).
    - Stable entrypoints used by UI dialogs and boot sequencing:
        * Get(...)
        * Boot()
        * LoadAudioConfig()
        * SaveAudioConfig()
        * ApplySettingsToRuntime()
        * LoadFromSaveGame() / WriteToSaveGame()
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StarshatterAudioSubsystem.generated.h"

class UGameInstance;
class UStarshatterAudioSettings;
class UStarshatterSettingsSaveGame;

UCLASS()
class STARSHATTERWARS_API UStarshatterAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // -----------------------------
    // UGameInstanceSubsystem
    // -----------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // -----------------------------
    // Boot entrypoint
    // -----------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void Boot();

    // -----------------------------
    // Stable API used by Boot + UI
    // -----------------------------

    /** Reload config from ini (CDO) and sanitize. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void LoadAudioConfig();

    /** Persist config (CDO) to ini. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void SaveAudioConfig();

    /** Apply current settings to runtime audio (SoundMix/SoundClass, etc.). */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void ApplySettingsToRuntime();

    /** Optional legacy alias if any old code expects this name. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void ApplySettingsToRuntimeAudio() { ApplySettingsToRuntime(); }

    /** Access underlying settings object (config-backed CDO). */
    UFUNCTION(BlueprintPure, Category = "Starshatter|Audio")
    UStarshatterAudioSettings* GetSettings() const;

    // -----------------------------
    // SaveGame bridging (unified settings file)
    // -----------------------------

    /** Pull Audio struct from the unified SaveGame into the AudioSettings CDO. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame);

    /** Push current AudioSettings CDO values into the unified SaveGame’s Audio struct. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Audio")
    void WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const;

    // -----------------------------
    // Convenience accessors
    // -----------------------------
    static UStarshatterAudioSubsystem* Get(const UObject* WorldContextObject);
    static UStarshatterAudioSubsystem* Get(UGameInstance* GameInstance);
};
