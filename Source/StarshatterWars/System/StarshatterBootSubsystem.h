/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterBootSubsystem.h
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
#include "StarshatterBootSubsystem.generated.h"

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


// NEW: Player save subsystem
class UStarshatterPlayerSubsystem;

// NEW: Forms subsystem
class UStarshatterFormSubsystem;

DECLARE_MULTICAST_DELEGATE(FOnStarshatterBootComplete);

UCLASS()
class STARSHATTERWARS_API UStarshatterBootSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool IsBootComplete() const { return bBootComplete; }
    FOnStarshatterBootComplete OnBootComplete;

    // If true, no player save existed when boot tried to load.
    bool NeedsFirstRun() const { return bNeedsFirstRun; }

    // Optional helper: triggers the GameData subsystem loader.
    // Recommended usage: call this from GameInitSubsystem during EGameMode::INIT.
    void BootGameDataLoader(bool bFull = false);
   

private:
    // --------------------------------------------------
    // Boot context (shared startup state)
    // --------------------------------------------------
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
    };

    bool BuildContext(FBootContext& OutCtx);

    void BootFonts(const FBootContext& Ctx);
    void BootAudio(const FBootContext& Ctx);
    void BootVideo(const FBootContext& Ctx);
    void BootControls(const FBootContext& Ctx);
    void BootKeyboard(const FBootContext& Ctx);

    //Game Data
    void BootShipDesignLoader(const FBootContext& Ctx);
    void BootGalaxyLoader(const FBootContext& Ctx);
    void BootSystemDesignLoader(const FBootContext& Ctx);
    void BootWeaponDesignLoader(const FBootContext& Ctx);

    // NEW:
    void BootPlayerSave(const FBootContext& Ctx);

    // NEW:
    void BootForms(const FBootContext& Ctx);

    void MarkBootComplete();

    void BootLegacyDataLoader(const FBootContext& Ctx);
    void IngestAllDesignData(bool bForceReimport);
    bool BootAssets();

    bool BootUI();
    void BootUIStyle(const FBootContext& Ctx);

private:
    bool bBootComplete = false;

    // NEW:
    bool bNeedsFirstRun = false;
};
