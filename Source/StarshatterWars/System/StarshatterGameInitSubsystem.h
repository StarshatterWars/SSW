/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         StarshatterGameInitSubsystem.h / .cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Post-boot game initialization coordinator.

    This subsystem executes AFTER the StarshatterBootSubsystem
    completes early GameInstance boot tasks (settings + runtime apply).

    It is responsible for runtime initialization that must occur after:
      - Settings have been loaded and applied (audio/video/input)
      - Core subsystems are active
      - Game data has been loaded/generated (via GameData subsystem)

    This subsystem intentionally does NOT perform:
      - SaveGame creation/loading (BootSubsystem owns this)
      - Audio, video, or input configuration (BootSubsystem owns this)
      - Any UI boot logic

    BOOT ORDER
    ==========
    1) StarshatterBootSubsystem
       - Loads/creates settings SaveGame
       - Applies audio/video/input settings
       - Broadcasts BootComplete

    2) StarshatterGameInitSubsystem (this)
       - Subscribes to BootComplete
       - Executes once per game session
       - Triggers GameData load
       - Performs runtime initialization
       - Transitions to MENU state

    DESIGN GOALS
    ============
    - Clear separation between BOOT and GAME INIT
    - Deterministic initialization order
    - No startup logic in UI code
    - No hidden dependencies or race conditions
    - Centralized ownership of runtime initialization

    OWNERSHIP RULES
    ===============
    - BootSubsystem is the sole authority for early startup
    - GameInitSubsystem assumes Boot is complete
    - Gameplay systems may rely on GameInit completion
    - No other system may perform runtime initialization

    This file is intentionally verbose in documentation.
    Runtime initialization order is critical and must remain explicit.
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StarshatterGameInitSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UStarshatterGameInitSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool IsGameInitComplete() const { return bGameInitComplete; }

private:
    void BeginAfterBoot();
    void RunGameInit();

    UPROPERTY(Transient)
    TArray<FName> CampaignRowOrder;

private:
    bool bGameInitComplete = false;
};
