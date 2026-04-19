/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         SystemLevelScriptActor.cpp
    AUTHOR:       Carlos Bott
*/

#include "SystemLevelScriptActor.h"

#include "SystemSceneBuilder.h"
#include "CampaignSceneActor.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "Campaign.h"
#include "Mission.h"
#include "StarSystem.h"

ASystemLevelScriptActor::ASystemLevelScriptActor()
{
    bInitializeOnBeginPlay = true;
    bBuildOnBeginPlay = true;
    bAutoFindBuilder = true;
    bAutoSpawnBuilderIfMissing = false;

    bAutoFindSceneActor = true;
    bAutoSpawnSceneActorIfMissing = true;
}

void ASystemLevelScriptActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] BeginPlay"));

    if (bInitializeOnBeginPlay)
    {
        InitializeSystemLevel();
    }

    // Ensure the scene actor exists for mission ship/station spawning.
    ResolveSceneActor();

    if (bBuildOnBeginPlay)
    {
        BuildCurrentSystem();
    }
}

void ASystemLevelScriptActor::InitializeSystemLevel()
{
    ASystemSceneBuilder* Builder = ResolveBuilder();
    if (!Builder)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemLevelScriptActor] InitializeSystemLevel: no builder"));
        return;
    }

    const FString SystemName = ResolveStartupSystemName();

    Builder->bBuildOnBeginPlay = false;

    // IMPORTANT:
    // This must be System, not Galaxy. "Solus" is a system name.
    Builder->DataSource = ESystemSceneSource::StarSystem;
    Builder->TargetSystemName = SystemName;
    Builder->TargetGalaxyName.Empty();

    UE_LOG(LogTemp, Log,
        TEXT("[SystemLevelScriptActor] Initialized builder for system '%s'"),
        *SystemName);
}

void ASystemLevelScriptActor::BuildCurrentSystem()
{
    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] BuildCurrentSystem called"));

    ASystemSceneBuilder* Builder = ResolveBuilder();
    if (!Builder)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemLevelScriptActor] BuildCurrentSystem: builder is null"));
        return;
    }

    // Ensure scene actor is present before the dialog tries to use it.
    ResolveSceneActor();

    if (Builder->DataSource != ESystemSceneSource::StarSystem || Builder->TargetSystemName.IsEmpty())
    {
        InitializeSystemLevel();
        Builder = ResolveBuilder();

        if (!Builder)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[SystemLevelScriptActor] BuildCurrentSystem: builder missing after init"));
            return;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] Calling Builder->BuildSystemScene()"));

    Builder->BuildSystemScene();
}

void ASystemLevelScriptActor::ClearCurrentSystem()
{
    ASystemSceneBuilder* Builder = ResolveBuilder();
    if (Builder)
    {
        Builder->ClearSpawnedBodies();
    }

    ACampaignSceneActor* SceneActor = ResolveSceneActor();
    if (SceneActor)
    {
        SceneActor->ClearSceneActors();
    }
}

void ASystemLevelScriptActor::RebuildCurrentSystem()
{
    ASystemSceneBuilder* Builder = ResolveBuilder();
    if (!Builder)
    {
        return;
    }

    Builder->ClearSpawnedBodies();

    ACampaignSceneActor* SceneActor = ResolveSceneActor();
    if (SceneActor)
    {
        SceneActor->ClearSceneActors();
    }

    Builder->BuildSystemScene();
}

FString ASystemLevelScriptActor::ResolveStartupSystemName() const
{
    const Campaign* CampaignPtr = Campaign::GetCampaign();

    if (CampaignPtr)
    {
        const Mission* MissionPtr = CampaignPtr->GetMission();
        if (MissionPtr)
        {
            StarSystem* Sys = MissionPtr->GetStarSystem();
            if (Sys && Sys->GetName() && *Sys->GetName())
            {
                const FString Name = ANSI_TO_TCHAR(Sys->GetName());

                UE_LOG(LogTemp, Log,
                    TEXT("[SystemLevelScriptActor] Resolved from Mission: %s"),
                    *Name);

                return Name;
            }
        }

        const List<StarSystem>& Systems = CampaignPtr->GetSystemList();
        if (Systems.size() > 0 && Systems[0])
        {
            const char* SysName = Systems[0]->GetName();
            if (SysName && *SysName)
            {
                const FString Name = ANSI_TO_TCHAR(SysName);

                UE_LOG(LogTemp, Log,
                    TEXT("[SystemLevelScriptActor] Resolved from Campaign list: %s"),
                    *Name);

                return Name;
            }
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] Falling back to default system: %s"),
        *DefaultSystemName);

    return DefaultSystemName;
}

ASystemSceneBuilder* ASystemLevelScriptActor::ResolveBuilder()
{
    if (BuilderOverride)
    {
        CachedBuilder = BuilderOverride;
        return CachedBuilder.Get();
    }

    if (CachedBuilder)
    {
        return CachedBuilder.Get();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (bAutoFindBuilder)
    {
        for (TActorIterator<ASystemSceneBuilder> It(World); It; ++It)
        {
            CachedBuilder = *It;

            UE_LOG(LogTemp, Log,
                TEXT("[SystemLevelScriptActor] Found existing builder: %s"),
                *CachedBuilder->GetName());

            return CachedBuilder.Get();
        }
    }

    if (!bAutoSpawnBuilderIfMissing)
    {
        return nullptr;
    }

    TSubclassOf<ASystemSceneBuilder> SpawnClass = BuilderClass;
    if (!SpawnClass)
    {
        SpawnClass = ASystemSceneBuilder::StaticClass();
    }

    const FString ResolvedSystemName = ResolveStartupSystemName();
    const FTransform SpawnTransform(
        BuilderSpawnRotation,
        GetActorLocation() + BuilderSpawnLocation);

    ASystemSceneBuilder* Spawned = World->SpawnActorDeferred<ASystemSceneBuilder>(
        SpawnClass,
        SpawnTransform,
        this,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemLevelScriptActor] Failed to deferred-spawn SystemSceneBuilder"));
        return nullptr;
    }

    Spawned->bBuildOnBeginPlay = false;

    // IMPORTANT:
    // This must be System, not Galaxy.
    Spawned->DataSource = ESystemSceneSource::StarSystem;
    Spawned->TargetSystemName = ResolvedSystemName;
    Spawned->TargetGalaxyName.Empty();

    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTransform);

    CachedBuilder = Spawned;

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] Auto-spawned builder: %s"),
        *Spawned->GetName());

    return CachedBuilder.Get();
}

ACampaignSceneActor* ASystemLevelScriptActor::ResolveSceneActor()
{
    if (SceneActorOverride)
    {
        CachedSceneActor = SceneActorOverride;
        return CachedSceneActor.Get();
    }

    if (CachedSceneActor)
    {
        return CachedSceneActor.Get();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (bAutoFindSceneActor)
    {
        for (TActorIterator<ACampaignSceneActor> It(World); It; ++It)
        {
            CachedSceneActor = *It;

            UE_LOG(LogTemp, Log,
                TEXT("[SystemLevelScriptActor] Found existing CampaignSceneActor: %s"),
                *CachedSceneActor->GetName());

            return CachedSceneActor.Get();
        }
    }

    if (!bAutoSpawnSceneActorIfMissing)
    {
        return nullptr;
    }

    TSubclassOf<ACampaignSceneActor> SpawnClass = SceneActorClass;
    if (!SpawnClass)
    {
        SpawnClass = ACampaignSceneActor::StaticClass();
    }

    const FTransform SpawnTransform(
        SceneActorSpawnRotation,
        GetActorLocation() + SceneActorSpawnLocation);

    ACampaignSceneActor* Spawned = World->SpawnActorDeferred<ACampaignSceneActor>(
        SpawnClass,
        SpawnTransform,
        this,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemLevelScriptActor] Failed to deferred-spawn CampaignSceneActor"));
        return nullptr;
    }

    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTransform);

    CachedSceneActor = Spawned;

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] Auto-spawned CampaignSceneActor: %s"),
        *Spawned->GetName());

    return CachedSceneActor.Get();
}