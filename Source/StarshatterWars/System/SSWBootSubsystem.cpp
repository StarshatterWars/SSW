/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         SSWBootSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Boot sequence coordinator for GameInstance-scoped systems.

    No world boot is performed here. No actors are spawned.
*/

#include "SSWBootSubsystem.h"

#include "Engine/GameInstance.h"

#include "GameStructs.h"
#include "GameStructs_UI.h"
#include "SSWGameInstance.h"

#include "DataLoader.h"

#include "FontManagerSubsystem.h"
#include "StarshatterAudioSubsystem.h"
#include "StarshatterVideoSubsystem.h"
#include "StarshatterControlsSubsystem.h"
#include "StarshatterKeyboardSubsystem.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "StarshatterSettingsSaveGame.h"

#include "StarshatterGameDataSubsystem.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterFormSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "SSWCombatGroupSubsystem.h"

#include "SSWGameInitSubsystem.h"
#include "SSWRuntimeSubsystem.h"


DEFINE_LOG_CATEGORY_STATIC(LogSSWBoot, Log, All);

void USSWBootSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] Initialize"));

    if (!DataLoader::GetLoader())
    {
        DataLoader::Initialize();
    }

    Collection.InitializeDependency(UStarshatterAssetRegistrySubsystem::StaticClass());
    Collection.InitializeDependency(UFontManagerSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterAudioSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterVideoSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterControlsSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterKeyboardSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterPlayerSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterFormSubsystem::StaticClass());

    Collection.InitializeDependency(UStarshatterGameDataSubsystem::StaticClass());

    Collection.InitializeDependency(UStarshatterShipDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterSystemDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterWeaponDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterEnvironmentSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterUIStyleSubsystem::StaticClass());
    Collection.InitializeDependency(USSWCombatGroupSubsystem::StaticClass());

    Collection.InitializeDependency(USSWRuntimeSubsystem::StaticClass());
    Collection.InitializeDependency(USSWGameInitSubsystem::StaticClass());

    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        SSWGI->SetGameMode(EGameMode::BOOT);
    }

    FBootContext Ctx;
    if (!BuildContext(Ctx))
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BuildContext failed; aborting boot."));
        return;
    }

    if (!BootAssets())
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootAssets failed; aborting boot."));
        return;
    }

    BootUIStyle(Ctx);

    if (!BootUI())
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootUI failed; aborting boot."));
        return;
    }

    if (BuildContext(Ctx))
    {
        BootLegacyDataLoader(Ctx);

        BootFonts(Ctx);
        BootAudio(Ctx);
        BootVideo(Ctx);
        BootControls(Ctx);
        BootKeyboard(Ctx);
        BootForms(Ctx);
        BootPlayerSave(Ctx);

        // Temporary behavior.
        // Later these get split into DEF parse vs table fill.
        BootSystemDesignLoader(Ctx);
        BootWeaponDesignLoader(Ctx);
        BootShipDesignLoader(Ctx);
        BootGalaxyLoader(Ctx);
        BootCombatGroupLoader(Ctx);
    }

    // Temporary passthrough.
    // Later move this fully into GameInit after DEF/table split.
    BootGameDataLoader(true);

    MarkBootComplete();

    // For now Boot explicitly kicks GameInit.
    StartGameInitSubsystem();
}

void USSWBootSubsystem::Deinitialize()
{
    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] Deinitialize"));

    Super::Deinitialize();
}

bool USSWBootSubsystem::BuildContext(FBootContext& OutCtx)
{
    OutCtx.GI = GetGameInstance();
    if (!OutCtx.GI)
        return false;

    OutCtx.SaveSS = OutCtx.GI->GetSubsystem<UStarshatterSettingsSaveSubsystem>();
    if (OutCtx.SaveSS)
    {
        OutCtx.SaveSS->LoadOrCreate();
        OutCtx.SG = OutCtx.SaveSS->GetSettings();
    }

    OutCtx.FontSS = OutCtx.GI->GetSubsystem<UFontManagerSubsystem>();
    OutCtx.AudioSS = OutCtx.GI->GetSubsystem<UStarshatterAudioSubsystem>();
    OutCtx.VideoSS = OutCtx.GI->GetSubsystem<UStarshatterVideoSubsystem>();
    OutCtx.ControlsSS = OutCtx.GI->GetSubsystem<UStarshatterControlsSubsystem>();
    OutCtx.KeyboardSS = OutCtx.GI->GetSubsystem<UStarshatterKeyboardSubsystem>();
    OutCtx.PlayerSS = OutCtx.GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    OutCtx.FormSS = OutCtx.GI->GetSubsystem<UStarshatterFormSubsystem>();

    OutCtx.ShipDesignSS = OutCtx.GI->GetSubsystem<UStarshatterShipDesignSubsystem>();
    OutCtx.SystemDesignSS = OutCtx.GI->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    OutCtx.WeaponDesignSS = OutCtx.GI->GetSubsystem<UStarshatterWeaponDesignSubsystem>();
    OutCtx.EnvironmentSS = OutCtx.GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    OutCtx.UIStyleSS = OutCtx.GI->GetSubsystem<UStarshatterUIStyleSubsystem>();
    OutCtx.CombatGroupSS = OutCtx.GI->GetSubsystem<USSWCombatGroupSubsystem>();

    return true;
}

void USSWBootSubsystem::BootLegacyDataLoader(const FBootContext& Ctx)
{
    if (!DataLoader::GetLoader())
    {
        UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] Initializing legacy DataLoader."));
        DataLoader::Initialize();
    }

    if (!DataLoader::GetLoader())
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] DataLoader initialization failed."));
        return;
    }

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] DataLoader ready."));
}

bool USSWBootSubsystem::BootAssets()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootAssets: No GameInstance."));
        return false;
    }

    UStarshatterAssetRegistrySubsystem* Assets =
        GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

    if (!Assets)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootAssets: AssetRegistry subsystem missing."));
        return false;
    }

    if (!Assets->InitRegistry())
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootAssets: InitRegistry failed."));
        return false;
    }

    const TArray<FName> Required =
    {
        TEXT("Data.WeaponDesignTable"),
        TEXT("Data.SystemDesignTable"),
        TEXT("Data.ShipDesignTable"),

        TEXT("Data.CampaignTable"),
        TEXT("Data.CampaignActionTable"),
        TEXT("Data.CampaignOOBTable"),
        TEXT("Data.CombatGroupTable"),
        TEXT("Data.GalaxyMapTable"),
        TEXT("Data.SystemMapTable"),

        TEXT("Data.OrderOfBattleTable"),
        TEXT("Data.MedalsTable"),
        TEXT("Data.RanksTable"),

        TEXT("Data.RegionsTable"),
        TEXT("Data.ZonesTable"),

        TEXT("UI.MenuScreenClass"),
        TEXT("UI.CampaignScreenClass"),
        TEXT("UI.CampaignSelectScreenClass"),
        TEXT("UI.CampaignLoadClass"),
        TEXT("UI.MissionSelectScreenClass"),
        TEXT("UI.ExitDlgClass"),
        TEXT("UI.FirstRunDlgClass"),
        TEXT("UI.OptionsScreenClass"),
        TEXT("UI.PlayerLogbookScreenClass"),
        TEXT("UI.TacRefScreenClass"),
        TEXT("UI.CmdMessageDlgClass"),
        TEXT("UI.OperationsScreenClass"),
        TEXT("UI.MissionScreenClass"),
        TEXT("UI.CampaignSceneClass"),

        TEXT("UI.Theme.MenuButton.Normal"),
        TEXT("UI.Theme.MenuButton.Hover"),
        TEXT("UI.Theme.MenuButton.Pressed"),
        TEXT("UI.Theme.MenuButton.Disabled")
    };

    if (!Assets->ValidateRequired(Required, true))
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootAssets: Required asset validation failed."));
        return false;
    }

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] BootAssets complete."));
    return true;
}

bool USSWBootSubsystem::BootUI()
{
    USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GetGameInstance());
    if (!SSWGI)
        return false;

    SSWGI->InitializeScreens();
    return true;
}

void USSWBootSubsystem::BootUIStyle(const FBootContext& Ctx)
{
    if (!Ctx.UIStyleSS)
        return;

    Ctx.UIStyleSS->ReloadFromSettings(true);

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] UIStyle loaded."));
}

void USSWBootSubsystem::BootFonts(const FBootContext& Ctx)
{
    if (!Ctx.FontSS)
        return;
}

void USSWBootSubsystem::BootAudio(const FBootContext& Ctx)
{
    if (!Ctx.AudioSS)
        return;

    if (Ctx.SG)
    {
        Ctx.AudioSS->LoadFromSaveGame(Ctx.SG);
    }

    Ctx.AudioSS->ApplySettingsToRuntime();
}

void USSWBootSubsystem::BootVideo(const FBootContext& Ctx)
{
    if (!Ctx.VideoSS)
        return;

    if (Ctx.SG)
    {
        Ctx.VideoSS->LoadFromSaveGame(Ctx.SG);
    }
    else
    {
        Ctx.VideoSS->LoadVideoConfig(TEXT("video.cfg"), true);
    }

    Ctx.VideoSS->ApplySettingsToRuntime();
}

void USSWBootSubsystem::BootControls(const FBootContext& Ctx)
{
    if (!Ctx.ControlsSS)
        return;

    if (Ctx.SG)
    {
        Ctx.ControlsSS->LoadFromSaveGame(Ctx.SG);
    }

    Ctx.ControlsSS->ApplySettingsToRuntime(this);
}

void USSWBootSubsystem::BootKeyboard(const FBootContext& Ctx)
{
    if (!Ctx.KeyboardSS)
        return;

    if (Ctx.SG)
    {
        Ctx.KeyboardSS->LoadFromSaveGame(Ctx.SG);
    }

    Ctx.KeyboardSS->ApplySettingsToRuntime(this);
}

void USSWBootSubsystem::BootForms(const FBootContext& Ctx)
{
    if (!Ctx.FormSS)
        return;

    // Ctx.FormSS->BootLoadForms();
}

void USSWBootSubsystem::BootPlayerSave(const FBootContext& Ctx)
{
    bNeedsFirstRun = false;

    if (!Ctx.PlayerSS)
        return;

    const bool bOk = Ctx.PlayerSS->LoadFromBoot();

    if (!bOk)
    {
        bNeedsFirstRun = true;
        return;
    }

    bNeedsFirstRun = !Ctx.PlayerSS->HadExistingSaveOnLoad();
}

void USSWBootSubsystem::BootSystemDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.SystemDesignSS)
        return;

    Ctx.SystemDesignSS->LoadAll(true);
}

void USSWBootSubsystem::BootWeaponDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.WeaponDesignSS)
        return;

    Ctx.WeaponDesignSS->LoadAll(false);
}

void USSWBootSubsystem::BootShipDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.ShipDesignSS)
        return;

    Ctx.ShipDesignSS->LoadAll(false);
}

void USSWBootSubsystem::BootGalaxyLoader(const FBootContext& Ctx)
{
    if (!Ctx.EnvironmentSS)
        return;

    Ctx.EnvironmentSS->LoadAll(false);
}

void USSWBootSubsystem::BootCombatGroupLoader(const FBootContext& Ctx)
{
    if (!Ctx.CombatGroupSS)
        return;

    Ctx.CombatGroupSS->LoadAll(false);
}

void USSWBootSubsystem::BootGameDataLoader(bool bFull)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootGameDataLoader: GameInstance null"));
        return;
    }

    UStarshatterGameDataSubsystem* DataSS =
        GI->GetSubsystem<UStarshatterGameDataSubsystem>();

    if (!DataSS)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] BootGameDataLoader: GameDataSubsystem missing"));
        return;
    }

    UE_LOG(LogSSWBoot, Warning,
        TEXT("[BOOT] BootGameDataLoader: CALL LoadAll(%s)"),
        bFull ? TEXT("TRUE") : TEXT("FALSE"));

    DataSS->LoadAll(bFull);

    UE_LOG(LogSSWBoot, Warning,
        TEXT("[BOOT] BootGameDataLoader: COMPLETE"));
}

void USSWBootSubsystem::IngestAllDesignData(bool bForceReimport)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    UStarshatterSystemDesignSubsystem* SysSS =
        GI->GetSubsystem<UStarshatterSystemDesignSubsystem>();

    UStarshatterWeaponDesignSubsystem* WepSS =
        GI->GetSubsystem<UStarshatterWeaponDesignSubsystem>();

    UStarshatterShipDesignSubsystem* ShipSS =
        GI->GetSubsystem<UStarshatterShipDesignSubsystem>();

    if (!SysSS || !WepSS || !ShipSS)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[INGEST] Missing required subsystem."));
        return;
    }

    SysSS->bClearTables = bForceReimport;
    WepSS->bClearTables = bForceReimport;
    ShipSS->bClearTables = bForceReimport;

    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] START FULL DESIGN INGESTION"));
    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] ForceReimport=%s"),
        bForceReimport ? TEXT("TRUE") : TEXT("FALSE"));

    SysSS->LoadSystemDesigns();
    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] SYSTEMS=%d"), SysSS->GetDesignsByName().Num());

    WepSS->LoadAll(false);
    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] WEAPONS=%d"), WepSS->GetDesignsByName().Num());

    ShipSS->LoadAll(false);
    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] SHIPS=%d"), ShipSS->GetDesignsByName().Num());

    UE_LOG(LogSSWBoot, Log, TEXT("[INGEST] END FULL DESIGN INGESTION"));
}

void USSWBootSubsystem::MarkBootComplete()
{
    if (bBootComplete)
        return;

    bBootComplete = true;

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] Complete. Broadcasting OnBootComplete."));

    OnBootComplete.Broadcast();
}

void USSWBootSubsystem::StartGameInitSubsystem()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    USSWGameInitSubsystem* GameInitSS =
        GI->GetSubsystem<USSWGameInitSubsystem>();

    if (!GameInitSS)
    {
        UE_LOG(LogSSWBoot, Error, TEXT("[BOOT] GameInit subsystem missing."));
        return;
    }

    UE_LOG(LogSSWBoot, Log, TEXT("[BOOT] Starting GameInit passthrough."));

    GameInitSS->RunGameInitFromBoot();
}