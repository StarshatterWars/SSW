/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterGameInitSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation for post-boot game initialization.

    This subsystem gates itself behind BootSubsystem completion,
    then triggers game data loading and performs runtime init.
*/

#include "StarshatterGameInitSubsystem.h"

#include "Engine/GameInstance.h"

// Boot coordinator:
#include "StarshatterBootSubsystem.h"

// Game data subsystem (replacement for AGameDataLoader actor):
#include "StarshatterGameDataSubsystem.h"

// Optional: if you want to set EGameMode here and your GI owns it:
#include "SSWGameInstance.h"

void UStarshatterGameInitSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BeginAfterBoot();
}

void UStarshatterGameInitSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UStarshatterGameInitSubsystem::BeginAfterBoot()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    UStarshatterBootSubsystem* BootSS = GI->GetSubsystem<UStarshatterBootSubsystem>();
    if (!BootSS)
        return;

    if (BootSS->IsBootComplete())
    {
        RunGameInit();
        return;
    }

    BootSS->OnBootComplete.AddUObject(this, &UStarshatterGameInitSubsystem::RunGameInit);
}

void UStarshatterGameInitSubsystem::RunGameInit()
{
    if (bGameInitComplete)
        return;

    bGameInitComplete = true;

    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    // --------------------------------------------------
    // OPTIONAL: Set EGameMode to INIT here if GI owns it
    // --------------------------------------------------
    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GI))
    {
        SSWGI->SetGameMode(EGameMode::INIT);
    }

    // --------------------------------------------------
    // 1) Trigger Game Data Load (Subsystem)
    // --------------------------------------------------
    if (UStarshatterGameDataSubsystem* DataSS = GI->GetSubsystem<UStarshatterGameDataSubsystem>())
    {
        // Use true only if you want "full generation" behavior:
        DataSS->LoadAll(true);
    }

    // --------------------------------------------------
    // 2) Runtime Initialization (your real init hooks)
    // --------------------------------------------------
    // Examples:
    // - Starshatter core runtime init
    // - Build registries/caches from loaded data
    // - Validate data and emit logs
    // - Initialize gameplay managers
    //
    // Keep this phase pure: do not duplicate Boot work.

    // --------------------------------------------------
    // OPTIONAL: Transition to MENU when ready
    // --------------------------------------------------
    if (USSWGameInstance* SSWGI = Cast<USSWGameInstance>(GI))
    {
         SSWGI->SetGameMode(EGameMode::MENU);
    }
}
