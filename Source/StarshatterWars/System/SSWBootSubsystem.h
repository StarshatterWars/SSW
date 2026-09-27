/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         SSWBootSubsystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Central bootstrap coordinator for Starshatter Wars.

    Responsible for deterministic initialization of all
    GameInstance-scoped systems at startup.

    This subsystem performs early boot tasks only:
      - Settings load
      - Audio apply
      - Video apply
      - Controls apply
      - Keyboard apply
      - Player Save load (FirstRun detection)
      - Fonts (optional)

    IMPORTANT
    =========
    This subsystem does NOT spawn Actors and does NOT require UWorld.
    Any heavy game data parsing/generation should be triggered after
    boot (typically during EGameMode::INIT) via the GameInit subsystem.

    GAME MODE OWNERSHIP
    ===================
    BootSubsystem sets EGameMode::BOOT at the start of Initialize().
    It does not advance beyond BOOT.

    BOOT COMPLETE
    =============
    BootComplete is broadcast once the boot sequence finishes.
    Systems that require a completed boot (ex: GameInitSubsystem)
    should subscribe to OnBootComplete.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SSWBootSubsystem.generated.h"

class UGameInstance;

class UStarshatterSettingsSaveSubsystem;
class UStarshatterSettingsSaveGame;
class UFontManagerSubsystem;
class UStarshatterAudioSubsystem;
class UStarshatterVideoSubsystem;
class UStarshatterControlsSubsystem;
class UStarshatterKeyboardSubsystem;
class UStarshatterGameDataSubsystem;
class UStarshatterShipDesignSubsystem;
class UStarshatterSystemDesignSubsystem;
class UStarshatterWeaponDesignSubsystem;
class UStarshatterAssetRegistrySubsystem;
class UStarshatterEnvironmentSubsystem;
class UStarshatterUIStyleSubsystem;
class UStarshatterPlayerSubsystem;
class UStarshatterFormSubsystem;
class USSWCombatGroupSubsystem;

DECLARE_MULTICAST_DELEGATE(FOnSSWBootComplete);

UCLASS()
class STARSHATTERWARS_API USSWBootSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool IsBootComplete() const { return bBootComplete; }
    bool NeedsFirstRun() const { return bNeedsFirstRun; }

    UFUNCTION(BlueprintCallable, Category = "SSW|Boot")
    bool ShouldRebuildTables() const
    {
        return bRebuildTables;
    }

    UFUNCTION(BlueprintCallable, Category = "SSW|Boot")
    void SetRebuildTables(bool bInRebuildTables)
    {
        bRebuildTables = bInRebuildTables;

        UE_LOG(LogTemp, Warning,
            TEXT("[SSWBootSubsystem] RebuildTables=%d"),
            bRebuildTables ? 1 : 0);
    }

    FOnSSWBootComplete OnBootComplete;

    // Temporary passthrough.
    // Later this should be split into:
    // 1. DEF parsing
    // 2. DataTable filling
    // 3. Runtime data init
    void BootGameDataLoader(bool bFull = false);

private:
    struct FBootContext
    {
        UGameInstance* GI = nullptr;

        UStarshatterSettingsSaveSubsystem* SaveSS = nullptr;
        UStarshatterSettingsSaveGame* SG = nullptr;

        UFontManagerSubsystem* FontSS = nullptr;
        UStarshatterAudioSubsystem* AudioSS = nullptr;
        UStarshatterVideoSubsystem* VideoSS = nullptr;
        UStarshatterControlsSubsystem* ControlsSS = nullptr;
        UStarshatterKeyboardSubsystem* KeyboardSS = nullptr;
        UStarshatterPlayerSubsystem* PlayerSS = nullptr;
        UStarshatterFormSubsystem* FormSS = nullptr;

        UStarshatterShipDesignSubsystem* ShipDesignSS = nullptr;
        UStarshatterSystemDesignSubsystem* SystemDesignSS = nullptr;
        UStarshatterWeaponDesignSubsystem* WeaponDesignSS = nullptr;
        UStarshatterEnvironmentSubsystem* EnvironmentSS = nullptr;
        UStarshatterUIStyleSubsystem* UIStyleSS = nullptr;
        USSWCombatGroupSubsystem* CombatGroupSS = nullptr;
    };

private:
    bool BuildContext(FBootContext& OutCtx);

    bool BootAssets();
    bool BootUI();

    void BootLegacyDataLoader(const FBootContext& Ctx);

    void BootFonts(const FBootContext& Ctx);
    void BootAudio(const FBootContext& Ctx);
    void BootVideo(const FBootContext& Ctx);
    void BootControls(const FBootContext& Ctx);
    void BootKeyboard(const FBootContext& Ctx);
    void BootForms(const FBootContext& Ctx);
    void BootPlayerSave(const FBootContext& Ctx);
    void BootUIStyle(const FBootContext& Ctx);

    void BootShipDesignLoader(const FBootContext& Ctx);
    void BootGalaxyLoader(const FBootContext& Ctx);
    void BootSystemDesignLoader(const FBootContext& Ctx);
    void BootWeaponDesignLoader(const FBootContext& Ctx);
    void BootCombatGroupLoader(const FBootContext& Ctx);

    void IngestAllDesignData(bool bForceReimport);

    void MarkBootComplete();
    void StartGameInitSubsystem();

private:
    bool bBootComplete = false;
    bool bNeedsFirstRun = false;
    bool bRebuildTables = true;
};