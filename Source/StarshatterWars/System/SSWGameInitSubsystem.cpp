/*
    Project Starshatter Wars
    Fractal Dev Studios

    FILE:         SSWGameInitSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Post-boot game initialization.

    Boot:
        Loads settings, assets, raw tables, and temporary data loaders.

    GameInit:
        Temporary passthrough coordinator.
        Calls RuntimeSubsystem startup.

    Runtime:
        Owns the actual Starshatter game loop.
*/

#include "SSWGameInitSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "SSWGameInstance.h"
#include "SSWRuntimeSubsystem.h"
#include "StarshatterGameDataSubsystem.h"
#include "DataLoader.h"
#include "FontManager.h"
#include "MusicController.h"
#include "TimerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSSWGameInit, Log, All);

void USSWGameInitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency(USSWRuntimeSubsystem::StaticClass());
    Collection.InitializeDependency(UStarshatterGameDataSubsystem::StaticClass());

    Super::Initialize(Collection);

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Initialize"));
}

void USSWGameInitSubsystem::Deinitialize()
{
    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Deinitialize"));

    Super::Deinitialize();
}

void USSWGameInitSubsystem::RunGameInitFromBoot()
{
    if (bGameInitStarted)
        return;

    bGameInitStarted = true;

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] RunGameInitFromBoot"));

    RunGameInit();
}

void USSWGameInitSubsystem::RunGameInit()
{
    if (bGameInitComplete)
        return;

    UGameInstance* BaseGI = GetGameInstance();
    if (!BaseGI)
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] No GameInstance."));
        return;
    }

    USSWGameInstance* SSWGI = Cast<USSWGameInstance>(BaseGI);
    if (!SSWGI)
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] GameInstance is not USSWGameInstance."));
        return;
    }

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Starting game initialization."));

    InitGameInstanceState();
    InitSaveSlots();
    InitGameTime();
    InitLegacyLoaderState();
    InitUniverseSave();
    BindRuntimeAutosave();
    InitMusicController();
    InitFonts();

    USSWRuntimeSubsystem* RuntimeSS =
        BaseGI->GetSubsystem<USSWRuntimeSubsystem>();

    if (!RuntimeSS)
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] Runtime subsystem missing."));
        return;
    }

    RuntimeSS->SetGameMode(EGameMode::INIT);

    if (!RuntimeSS->Init())
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] Runtime Init failed."));
        return;
    }

    if (!RuntimeSS->InitGame())
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] Runtime InitGame failed."));
        return;
    }

    RuntimeSS->StartRuntime();
    RuntimeSS->SetGameMode(EGameMode::MENU);

    bGameInitComplete = true;

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Complete. Runtime started."));
}

void USSWGameInitSubsystem::InitGameInstanceState()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    SetWindowed(false);
    SetGameActive(false);
    SetDeviceLost(false);
    SetMinimized(false);
    SetMaximized(false);
    SetIgnoreSizeChange(false);
    SetDeviceInitialized(false);
    SetDeviceRestored(false);

    SetGameStatus(EGAMESTATUS::OK);

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] GameInstance state initialized."));
}

void USSWGameInitSubsystem::InitSaveSlots()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    GI->PlayerSaveName = TEXT("PlayerSaveSlot");
    GI->PlayerSaveSlot = 0;

    GI->UniverseSaveSlotName = TEXT("Universe_Main");
    GI->UniverseSaveUserIndex = 0;

    GI->CampaignSaveSlotName = TEXT("Campaign");
    GI->CampaignSaveIndex = 0;

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Save slots initialized."));
}

void USSWGameInitSubsystem::InitGameTime()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    const FDateTime GameDate(2228, 1, 1);

    GI->SetGameTime(GameDate.ToUnixTimestamp());
    GI->SetCampaignTime(0);

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Game time initialized."));
}

void USSWGameInitSubsystem::InitLegacyLoaderState()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    GI->CampaignData.SetNum(5);
    GI->loader = DataLoader::GetLoader();

    UE_LOG(LogSSWGameInit, Log,
        TEXT("[GAMEINIT] Legacy loader state initialized. Loader=%p"),
        GI->loader);
}

void USSWGameInitSubsystem::InitUniverseSave()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    GI->LoadOrCreateUniverse();

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Universe save initialized."));
}

void USSWGameInitSubsystem::BindRuntimeAutosave()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>();
    if (!Timer)
    {
        UE_LOG(LogSSWGameInit, Warning, TEXT("[GAMEINIT] TimerSubsystem missing; autosave not bound."));
        return;
    }

    Timer->OnUniverseMinute.RemoveAll(GI);
    Timer->OnUniverseMinute.AddUObject(
        GI,
        &USSWGameInstance::HandleUniverseMinuteAutosave);

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Runtime autosave bound."));
}

void USSWGameInitSubsystem::InitMusicController()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    GI->SetupMusicController();

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Music controller initialized."));
}

void USSWGameInitSubsystem::InitFonts()
{
    USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI)
        return;

    FontManager::RegisterAllFonts(GI);

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Fonts registered."));
}

