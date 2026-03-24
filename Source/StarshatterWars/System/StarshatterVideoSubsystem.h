/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterVideoSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UStarshatterVideoSubsystem
    - GameInstanceSubsystem that owns legacy video.cfg load/save and an in-memory config struct.
    - Stable entrypoints used by UI dialogs + boot sequencing:
        * Get(...)
        * Boot()
        * LoadVideoConfig()
        * SaveVideoConfig()
        * ApplySettingsToRuntime()
        * LoadFromSaveGame() / WriteToSaveGame()
    - Keeps UI + boot from directly parsing files.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameStructs.h"
#include "StarshatterVideoSubsystem.generated.h"

class UGameInstance;
class UStarshatterSettingsSaveGame;

// +--------------------------------------------------------------------+
// Delegates
// +--------------------------------------------------------------------+

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnStarshatterVideoChangeRequested,
    const FStarshatterVideoConfig&,
    PendingConfig
);

// +--------------------------------------------------------------------+
// Video Subsystem
// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UStarshatterVideoSubsystem : public UGameInstanceSubsystem
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
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void Boot();

    // -----------------------------
    // Stable API used by Boot + UI
    // -----------------------------

    /** Loads video.cfg into CurrentConfig (optionally creates file if missing). */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    bool LoadVideoConfig(const FString& InRelativeOrAbsolutePath = TEXT("video.cfg"), bool bCreateIfMissing = true);

    /** Saves CurrentConfig to video.cfg. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    bool SaveVideoConfig(const FString& InRelativeOrAbsolutePath = TEXT("video.cfg")) const;

    /** Applies CURRENT config to runtime (kept no-arg so UI callsites are simple). */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void ApplySettingsToRuntime();

    // -----------------------------
    // Current Config Access
    // -----------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|Video")
    const FStarshatterVideoConfig& GetConfig() const { return CurrentConfig; }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void SetConfig(const FStarshatterVideoConfig& NewConfig);

    // -----------------------------
    // Deferred Change Handling
    // -----------------------------

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void RequestChangeVideo(const FStarshatterVideoConfig& NewPendingConfig);

    UFUNCTION(BlueprintPure, Category = "Starshatter|Video")
    bool HasPendingChange() const { return bPendingChange; }

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    bool ConsumePendingChange(FStarshatterVideoConfig& OutPending);

    // -----------------------------
    // SaveGame bridging (unified settings file)
    // -----------------------------

    /** Pull Video struct from unified SaveGame into subsystem CurrentConfig. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void LoadFromSaveGame(const UStarshatterSettingsSaveGame* SaveGame);

    /** Push subsystem CurrentConfig into unified SaveGame’s Video struct. */
    UFUNCTION(BlueprintCallable, Category = "Starshatter|Video")
    void WriteToSaveGame(UStarshatterSettingsSaveGame* SaveGame) const;

    // -----------------------------
    // Convenience accessors
    // -----------------------------
    static UStarshatterVideoSubsystem* Get(const UObject* WorldContextObject);
    static UStarshatterVideoSubsystem* Get(UGameInstance* GameInstance);

public:
    // -----------------------------
    // Events
    // -----------------------------
    UPROPERTY(BlueprintAssignable, Category = "Starshatter|Video")
    FOnStarshatterVideoChangeRequested OnVideoChangeRequested;

public:
    // -----------------------------
    // Path Helpers
    // -----------------------------
    UFUNCTION(BlueprintPure, Category = "Starshatter|Video")
    FString GetDefaultConfigDir() const;

    UFUNCTION(BlueprintPure, Category = "Starshatter|Video")
    FString ResolveConfigPath(const FString& InRelativeOrAbsolutePath) const;

private:
    // -----------------------------
    // Internal State
    // -----------------------------
    UPROPERTY(Transient)
    FStarshatterVideoConfig CurrentConfig;

    UPROPERTY(Transient)
    FStarshatterVideoConfig PendingConfig;

    UPROPERTY(Transient)
    bool bPendingChange = false;

private:
    // -----------------------------
    // Parsing Helpers
    // -----------------------------
    static bool ParseBool(const FString& Value, bool& OutBool);
    static bool ParseInt(const FString& Value, int32& OutInt);
    static bool ParseFloat(const FString& Value, float& OutFloat);

    static void WriteLine(TArray<FString>& Lines, const FString& Key, const FString& Value);

private:
    // Centralized clamping/sanitize for safety:
    void SanitizeConfig(FStarshatterVideoConfig& C) const;
};
