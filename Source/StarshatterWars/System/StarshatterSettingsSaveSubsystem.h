/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         StarshatterSettingsSaveSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterSettingsSaveSubsystem

    Persistence-only GameInstance subsystem:
      - Loads/creates UStarshatterSettingsSaveGame
      - Runs version migration hooks
      - Caches settings for UI + subsystems
      - Saves on demand and optionally on Deinitialize
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StarshatterSettingsSaveSubsystem.generated.h"

class UStarshatterSettingsSaveGame;

UCLASS()
class STARSHATTERWARS_API UStarshatterSettingsSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    // ----------------------------------------------------------------
    // Preferred API Names (what you asked for)
    // ----------------------------------------------------------------

    /** Loads settings or creates slot if missing; runs migration; caches result. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool LoadOrCreateSettings();

    /** Saves cached settings (sanitizes first). */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool SaveSettings();

public:
    // ----------------------------------------------------------------
    // Back-compat API (optional; keep if already used)
    // ----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool LoadOrCreate() { return LoadOrCreateSettings(); }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool Save() { return SaveSettings(); }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    bool Reload();

    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    UStarshatterSettingsSaveGame* GetSettings() const { return CachedSettings; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    bool HasSettings() const { return CachedSettings != nullptr; }

public:
    // ----------------------------------------------------------------
    // Slot helpers
    // ----------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    FString GetSlotName() const;

    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    int32 GetUserIndex() const;

public:
    // ----------------------------------------------------------------
    // Dirty / autosave policy
    // ----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Settings")
    void MarkDirty() { bDirty = true; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|Settings")
    bool IsDirty() const { return bDirty; }

    /** If enabled, subsystem will attempt a save during Deinitialize if dirty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings")
    bool bSaveOnDeinitialize = true;

    /** If true, Initialize() will auto-load/create settings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Starshatter|Settings")
    bool bAutoLoadOnInitialize = true;

private:
    bool LoadInternal(bool bCreateIfMissing);

    /** Runs savegame migration hook and sanitization; marks dirty if changed. */
    void MigrateAndSanitize();

private:
    UPROPERTY(Transient)
    TObjectPtr<UStarshatterSettingsSaveGame> CachedSettings = nullptr;

    bool bDirty = false;
};
