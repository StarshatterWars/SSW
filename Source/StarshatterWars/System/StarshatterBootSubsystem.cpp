/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterBootSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Boot sequence coordinator for GameInstance-scoped systems.

    No world boot is performed here. No actors are spawned.
*/

#include "StarshatterBootSubsystem.h"

#include "Engine/GameInstance.h"

// Game mode:
#include "GameStructs.h"
#include "GameStructs_UI.h"
#include "SSWGameInstance.h"

// Subsystems
#include "FontManagerSubsystem.h"
#include "StarshatterAudioSubsystem.h"
#include "StarshatterVideoSubsystem.h"
#include "StarshatterControlsSubsystem.h"
#include "StarshatterKeyboardSubsystem.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "DataLoader.h"

// SaveGame
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

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterBoot, Log, All);

void UStarshatterBootSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!DataLoader::GetLoader())
        DataLoader::Initialize();

    DataLoader::GetLoader();
     // Force creation order (and guarantee non-null during BOOT):
    Collection.InitializeDependency(UStarshatterAssetRegistrySubsystem::StaticClass());
    Collection.InitializeDependency(UFontManagerSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterAudioSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterVideoSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterControlsSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterKeyboardSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterPlayerSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterFormSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterShipDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterSystemDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterWeaponDesignSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterEnvironmentSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterUIStyleSubsystem::StaticClass());

    // Establish BOOT lifecycle state immediately
    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        SSWGI->SetGameMode(EGameMode::BOOT);
    }

    FBootContext Ctx;
    if (!BuildContext(Ctx))
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BuildContext failed; aborting boot"));
        return;
    }

    if (!BootAssets())
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BootAssets failed; aborting boot"));
        return;
    }

    if (Ctx.UIStyleSS)
    {
        Ctx.UIStyleSS->ReloadFromSettings(true);
    }

    if (!BootUI())
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BootUI failed; aborting boot"));
        return;
    }

    if(BuildContext(Ctx))
    {
        BootLegacyDataLoader(Ctx);
        BootFonts(Ctx);
        BootAudio(Ctx);
        BootVideo(Ctx);
        BootControls(Ctx);
        BootKeyboard(Ctx);
        BootForms(Ctx);        

        BootPlayerSave(Ctx);
       
        BootSystemDesignLoader(Ctx);
        BootWeaponDesignLoader(Ctx);
        BootShipDesignLoader(Ctx);
        BootGalaxyLoader(Ctx);
    }

    // Keep existing behavior
    BootGameDataLoader(true);
    MarkBootComplete();
}

void UStarshatterBootSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UStarshatterBootSubsystem::BootGameDataLoader(bool bFull)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    UStarshatterGameDataSubsystem* DataSS =
        GI->GetSubsystem<UStarshatterGameDataSubsystem>();

    if (!DataSS)
        return;

    DataSS->LoadAll(bFull);
}

void UStarshatterBootSubsystem::BootShipDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.ShipDesignSS)
        return;

    Ctx.ShipDesignSS->LoadAll(true);
}

void UStarshatterBootSubsystem::BootGalaxyLoader(const FBootContext& Ctx)
{
    if (!Ctx.EnvironmentSS)
        return;

    Ctx.EnvironmentSS->LoadAll(true);
}

void UStarshatterBootSubsystem::BootSystemDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.SystemDesignSS)
        return;

    Ctx.SystemDesignSS->LoadAll(true);
}

void UStarshatterBootSubsystem::BootWeaponDesignLoader(const FBootContext& Ctx)
{
    if (!Ctx.WeaponDesignSS)
        return;

    Ctx.WeaponDesignSS->LoadAll(true);
}


bool UStarshatterBootSubsystem::BuildContext(FBootContext& OutCtx)
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
    OutCtx.ShipDesignSS = OutCtx.GI->GetSubsystem<UStarshatterShipDesignSubsystem>();
    OutCtx.SystemDesignSS = OutCtx.GI->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    OutCtx.WeaponDesignSS = OutCtx.GI->GetSubsystem<UStarshatterWeaponDesignSubsystem>();
    OutCtx.EnvironmentSS = OutCtx.GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();
    OutCtx.UIStyleSS = OutCtx.GI->GetSubsystem<UStarshatterUIStyleSubsystem>();
   
    OutCtx.FormSS = OutCtx.GI->GetSubsystem<UStarshatterFormSubsystem>();

    return true;
}

void UStarshatterBootSubsystem::BootFonts(const FBootContext& Ctx)
{
    if (!Ctx.FontSS)
        return;
}

void UStarshatterBootSubsystem::BootAudio(const FBootContext& Ctx)
{
    if (!Ctx.AudioSS)
        return;

    if (Ctx.SG)
        Ctx.AudioSS->LoadFromSaveGame(Ctx.SG);

    Ctx.AudioSS->ApplySettingsToRuntime();
}

void UStarshatterBootSubsystem::BootVideo(const FBootContext& Ctx)
{
    if (!Ctx.VideoSS)
        return;

    if (Ctx.SG)
        Ctx.VideoSS->LoadFromSaveGame(Ctx.SG);
    else
        Ctx.VideoSS->LoadVideoConfig(TEXT("video.cfg"), true);

    Ctx.VideoSS->ApplySettingsToRuntime();
}

void UStarshatterBootSubsystem::BootControls(const FBootContext& Ctx)
{
    if (!Ctx.ControlsSS)
        return;

    if (Ctx.SG)
        Ctx.ControlsSS->LoadFromSaveGame(Ctx.SG);

    Ctx.ControlsSS->ApplySettingsToRuntime(this);
}

void UStarshatterBootSubsystem::BootKeyboard(const FBootContext& Ctx)
{
    if (!Ctx.KeyboardSS)
        return;

    if (Ctx.SG)
        Ctx.KeyboardSS->LoadFromSaveGame(Ctx.SG);

    Ctx.KeyboardSS->ApplySettingsToRuntime(this);
}

void UStarshatterBootSubsystem::BootForms(const FBootContext& Ctx)
{
    if (!Ctx.FormSS)
        return;

    //Ctx.FormSS->BootLoadForms();
}

void UStarshatterBootSubsystem::BootPlayerSave(const FBootContext& Ctx)
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

void UStarshatterBootSubsystem::MarkBootComplete()
{
    if (bBootComplete)
        return;

    bBootComplete = true;
    OnBootComplete.Broadcast();
}

void UStarshatterBootSubsystem::BootLegacyDataLoader(const FBootContext& Ctx)
{
    if (!DataLoader::GetLoader())
    {
        UE_LOG(LogStarshatterBoot, Log, TEXT("[BOOT] Initializing legacy DataLoader..."));
        DataLoader::Initialize();
    }

    if (!DataLoader::GetLoader())
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] DataLoader::Initialize failed; loader still null."));
    }
    else
    {
        UE_LOG(LogStarshatterBoot, Log, TEXT("[BOOT] DataLoader ready."));
    }
}

void UStarshatterBootSubsystem::IngestAllDesignData(bool bForceReimport)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    auto* SysSS = GI->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    auto* WepSS = GI->GetSubsystem<UStarshatterWeaponDesignSubsystem>();
    auto* ShipSS = GI->GetSubsystem<UStarshatterShipDesignSubsystem>();

    if (!SysSS || !WepSS || !ShipSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[INGEST] Missing required subsystem(s)."));
        return;
    }

    // Optional: clear-only when forcing full rebuild
    SysSS->bClearTables = bForceReimport;
    WepSS->bClearTables = bForceReimport;
    ShipSS->bClearTables = bForceReimport;

    UE_LOG(LogTemp, Log, TEXT("[INGEST] ------------------------------"));
    UE_LOG(LogTemp, Log, TEXT("[INGEST] START FULL DESIGN INGESTION"));
    UE_LOG(LogTemp, Log, TEXT("[INGEST] ForceReimport = %s"), bForceReimport ? TEXT("TRUE") : TEXT("FALSE"));

    // 1) SYSTEMS
    SysSS->LoadSystemDesigns();
    UE_LOG(LogTemp, Log, TEXT("[INGEST] SYSTEMS: %d"), SysSS->GetDesignsByName().Num());

    // 2) WEAPONS
    WepSS->LoadWeaponDesigns();
    UE_LOG(LogTemp, Log, TEXT("[INGEST] WEAPONS: %d"), WepSS->GetDesignsByName().Num());

    // 3) SHIPS
    ShipSS->LoadShipDesigns(); // your existing scan + parse
    UE_LOG(LogTemp, Log, TEXT("[INGEST] SHIPS: %d"), ShipSS->GetDesignsByName().Num());

    UE_LOG(LogTemp, Log, TEXT("[INGEST] END FULL DESIGN INGESTION"));
    UE_LOG(LogTemp, Log, TEXT("[INGEST] ------------------------------"));
}

bool UStarshatterBootSubsystem::BootAssets()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BootAssets: No GameInstance"));
        return false;
    }

    UStarshatterAssetRegistrySubsystem* Assets =
        GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

    if (!Assets)
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BootAssets: AssetRegistry subsystem missing"));
        return false;
    }

    if (!Assets->InitRegistry())
    {
        UE_LOG(LogStarshatterBoot, Error, TEXT("[BOOT] BootAssets: InitRegistry failed"));
        return false;
    }

    const TArray<FName> Required = {
    TEXT("Data.WeaponDesignTable"),
    TEXT("Data.SystemDesignTable"),
    TEXT("Data.ShipDesignTable"),

    TEXT("Data.CampaignTable"),
    TEXT("Data.CampaignActionTable"),
    TEXT("Data.CampaignOOBTable"),
    TEXT("Data.CombatGroupTable"),
    TEXT("Data.GalaxyMapTable"),
    TEXT("Data.OrderOfBattleTable"),
    TEXT("Data.MedalsTable"),
    TEXT("Data.RanksTable"),

    TEXT("Data.RegionsTable"),
    TEXT("Data.ZonesTable"),

    TEXT("UI.MenuScreenClass"),
    TEXT("UI.CampaignSelectScreenClass"),
    TEXT("UI.MissionSelectScreenClass"),
    TEXT("UI.ExitDlgClass"),
    TEXT("UI.FirstRunDlgClass"),
    TEXT("UI.OptionsScreenClass"),
    TEXT("UI.PlayerLogbookScreenClass"),
    TEXT("UI.TacRefScreenClass"),
    TEXT("UI.OptionsScreenClass"),
   

    TEXT("UI.Theme.MenuButton.Normal"),
    TEXT("UI.Theme.MenuButton.Hover"),
    TEXT("UI.Theme.MenuButton.Pressed"),
    TEXT("UI.Theme.MenuButton.Disabled"),
    };

    if (!Assets->ValidateRequired(Required, /*bLoadNow=*/true))
        return false;

    UE_LOG(LogStarshatterBoot, Log, TEXT("[BOOT] BootAssets: OK"));
        return true;
}

bool UStarshatterBootSubsystem::BootUI()
{
    USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GetGameInstance());
    if (!SSWGI)
        return false;

    SSWGI->InitializeScreens();
    return true;
}

void UStarshatterBootSubsystem::BootUIStyle(const FBootContext& Ctx)
{
    if (!Ctx.UIStyleSS)
        return;

    Ctx.UIStyleSS->ReloadFromSettings(true);

    UE_LOG(LogStarshatterBoot, Log, TEXT("[BOOT] UIStyle loaded from Project Settings"));
}