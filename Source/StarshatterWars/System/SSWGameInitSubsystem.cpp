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

#include "SSWGameInstance.h"
#include "SSWRuntimeSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSSWGameInit, Log, All);

void USSWGameInitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency(USSWRuntimeSubsystem::StaticClass());

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

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] No GameInstance."));
        return;
    }

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Starting runtime initialization."));

    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GI))
    {
        SSWGI->SetGameMode(EGameMode::INIT);
    }

    USSWRuntimeSubsystem* RuntimeSS =
        GI->GetSubsystem<USSWRuntimeSubsystem>();

    if (!RuntimeSS)
    {
        UE_LOG(LogSSWGameInit, Error, TEXT("[GAMEINIT] Runtime subsystem missing."));
        return;
    }

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

    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GI))
    {
        SSWGI->SetGameMode(EGameMode::MENU);
    }

    bGameInitComplete = true;

    UE_LOG(LogSSWGameInit, Log, TEXT("[GAMEINIT] Complete. Runtime started."));
}