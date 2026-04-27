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

    // Ensure scene actor exists (but do NOT drive it here)
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
        TEXT("[SystemLevelScriptActor] BEFORE Builder->BuildSystemScene()"));

    Builder->BuildSystemScene();

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] AFTER Builder->BuildSystemScene()"));

    // IMPORTANT:
    // DO NOT attempt to spawn ships here.
    // Scene ships must be spawned from SceneDlg / scene transition logic,
    // because Campaign::GetMission() is not valid in this flow.

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemLevelScriptActor] System build complete (ships handled externally)"));
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
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemLevelScriptActor] RebuildCurrentSystem: builder is null"));
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
                return ANSI_TO_TCHAR(Sys->GetName());
            }
        }
    }

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

    return nullptr;
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

    return nullptr;
}