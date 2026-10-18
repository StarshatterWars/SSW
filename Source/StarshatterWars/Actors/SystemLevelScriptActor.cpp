#include "SystemLevelScriptActor.h"
#include "Engine/Engine.h"

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

    UE_LOG(LogTemp, Error, TEXT("[SSW BUILD PROBE 20261018] LEVEL BEGINPLAY World=%s Actor=%s"),
        *GetNameSafe(GetWorld()), *GetNameSafe(this));
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(990101, 20.0f, FColor::Red,
            TEXT("SSW BUILD PROBE 20261018: SYSTEM LEVEL BEGINPLAY"));
    }


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
    UE_LOG(LogTemp, Error, TEXT("[SSW BUILD PROBE 20261018] LEVEL BUILD CURRENT SYSTEM"));
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(990102, 20.0f, FColor::Yellow,
            TEXT("SSW BUILD PROBE 20261018: SYSTEM BUILD CALLED"));

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

bool ASystemLevelScriptActor::EnsureCurrentSystemBuilt()
{
    ASystemSceneBuilder* Builder = ResolveBuilder();
    if (!IsValid(Builder))
    {
        UE_LOG(LogTemp, Error, TEXT("[PlanetPreflight] No persistent SystemSceneBuilder"));
        return false;
    }

    const FString RequiredSystem = ResolveStartupSystemName().TrimStartAndEnd();
    if (RequiredSystem.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[PlanetPreflight] No mission StarSystem name"));
        return false;
    }

    auto CountPlanets = [Builder]() -> int32
    {
        int32 Count = 0;
        for (const FSpawnedSystemBody& Body : Builder->GetSpawnedBodies())
        {
            if (IsValid(Body.Actor.Get()) && !Body.bIsOrbit &&
                !Body.bIsMoon && !Body.BodyName.Equals(TEXT("CentralSun"), ESearchCase::IgnoreCase))
            {
                ++Count;
            }
        }
        return Count;
    };

    const bool bSameSystem =
        Builder->GetLastResolvedSystemName().Equals(RequiredSystem, ESearchCase::IgnoreCase);
    const int32 ExistingPlanetCount = CountPlanets();
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetPreflight] Builder=%s Required=%s LastResolved=%s ExistingPlanets=%d"),
        *GetNameSafe(Builder), *RequiredSystem,
        *Builder->GetLastResolvedSystemName(), ExistingPlanetCount);

    if (bSameSystem && ExistingPlanetCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlanetPreflight] Reusing existing planetary scene"));
        return true;
    }

    // Mirror InitializeSystemLevel without reading a potentially stale system selection.
    Builder->bBuildOnBeginPlay = false;
    Builder->DataSource = ESystemSceneSource::StarSystem;
    Builder->TargetSystemName = RequiredSystem;
    Builder->TargetGalaxyName.Empty();
    Builder->BuildSystemScene();

    const int32 BuiltPlanets = CountPlanets();
    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetPreflight] AFTER Build: Resolved=%s Bodies=%d Planets=%d"),
        *Builder->GetLastResolvedSystemName(), Builder->GetSpawnedBodies().Num(), BuiltPlanets);
    return Builder->GetLastResolvedSystemName().Equals(RequiredSystem, ESearchCase::IgnoreCase)
        && BuiltPlanets > 0;
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