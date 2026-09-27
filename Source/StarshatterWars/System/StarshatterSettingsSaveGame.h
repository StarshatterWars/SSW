/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterSettingsSaveGame.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterSettingsSaveGame

    Single SaveGame container for user settings:
      - Audio
      - Video
      - Controls (non-bindings)
      - Enhanced Input KeyMap (FKey-based bindings)

    Notes:
      - This is intentionally UE-native and does NOT depend on legacy Starshatter key.cfg.
      - Subsystems (Audio/Video/Controls) should read/write this SaveGame, but this class is
        deliberately lightweight and data-only (with a few convenience helpers).

    VERSIONING
    ==========
      - CurrentVersion: compile-time constant representing the latest save schema
      - SaveVersion: stored in the save file (replaces/aligns with SettingsVersion)
      - MigrateToCurrentVersion(): hook for future upgrades
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

// Your shared settings structs live here:
#include "GameStructs.h"

#include "StarshatterSettingsSaveGame.generated.h"

UCLASS()
class STARSHATTERWARS_API UStarshatterSettingsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UStarshatterSettingsSaveGame();

    // ----------------------------------------------------------------
    // Versioning (required by SettingsSaveSubsystem)
    // ----------------------------------------------------------------

    /** Increment when you change schema/meaning of saved fields. */
    static constexpr int32 CurrentVersion = 1;

    /**
        Version stored in the save file.
        This is what the SaveSubsystem will check/migrate.
    */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Starshatter|Settings")
    int32 SaveVersion = CurrentVersion;

    // ----------------------------------------------------------------
    // Metadata (keep for UI/debug; kept in sync with SaveVersion)
    // ----------------------------------------------------------------

    /** Legacy name kept for clarity in UI/blueprints; mirrors SaveVersion. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Starshatter|Settings")
    int32 SettingsVersion = CurrentVersion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Starshatter|Settings")
    FDateTime LastSavedUtc;

    // ----------------------------------------------------------------
    // Settings Payload
    // ----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings|Audio")
    FStarshatterAudioConfig Audio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings|Video")
    FStarshatterVideoConfig Video;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings|Controls")
    FStarshatterControlsConfig Controls;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings|Input")
    FStarshatterKeyMap KeyMap;

public:
    // ----------------------------------------------------------------
    // Convenience
    // ----------------------------------------------------------------

    /** Call before saving to clamp/sanitize. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    void Sanitize();

    /**
        Migrate older saves up to CurrentVersion.
        Returns true if any changes were made.
    */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool MigrateToCurrentVersion();

    /** Returns a consistent default slot name for settings. */
    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    static FString GetDefaultSlotName();

    /** Returns a consistent default user index for settings. */
    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    static int32 GetDefaultUserIndex();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings")
    FStarshatterKeyboardConfig KeyboardConfig;
};
