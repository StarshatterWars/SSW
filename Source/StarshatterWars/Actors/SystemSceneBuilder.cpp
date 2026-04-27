/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         SystemSceneBuilder.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Runtime builder for a star system scene.

    Uses live runtime StarSystem data from StarSystemRegistry and anchors
    the primary star at the builder origin. Planets and moons update every tick.
*/

#include "SystemSceneBuilder.h"

#include "SystemUtils.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystemRegistry.h"
#include "SystemUtils.h"

#include "StarSystem.h"
#include "Orbital.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "PlanetActor.h"
#include "GasGiantActor.h"

#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include "Misc/PackageName.h"

#include "Engine/SkyLight.h"
#include "Engine/GameInstance.h"
#include "Engine/DirectionalLight.h"
#include "Engine/TextureCube.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

ASystemSceneBuilder::ASystemSceneBuilder()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.TickInterval = 0.0f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SceneRoot->SetMobility(EComponentMobility::Movable);
    RootComponent = SceneRoot;

    // ----------------------------------------------------
    // DEFAULT ACTOR CLASSES (fallbacks if not set in editor)
    // ----------------------------------------------------
    PlanetActorClass = APlanetActor::StaticClass();
    TerranPlanetActorClass = APlanetActor::StaticClass();
    BarrenPlanetActorClass = APlanetActor::StaticClass();
    IcePlanetActorClass = APlanetActor::StaticClass();
    VolcanicPlanetActorClass = APlanetActor::StaticClass();
    MoonActorClass = APlanetActor::StaticClass();

    // Gas giant should stay its own class
    GasGiantPlanetActorClass = AGasGiantActor::StaticClass();

    bBuildOnBeginPlay = false;
    DataSource = ESystemSceneSource::Galaxy;
}

void ASystemSceneBuilder::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    if (bRebuildInEditor && !HasAnyFlags(RF_ClassDefaultObject))
    {
        BuildSystemScene();
    }
#endif
}

void ASystemSceneBuilder::BeginPlay()
{
    Super::BeginPlay();

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BeginPlay Builder=%s TickEnabled=%s DataSource=%d TargetGalaxy='%s' TargetSystem='%s' ScaleMode=%d"),
            *GetName(),
            PrimaryActorTick.bCanEverTick ? TEXT("true") : TEXT("false"),
            (int32)DataSource,
            *TargetGalaxyName,
            *TargetSystemName,
            (int32)ScaleSettings.ScaleMode);
    }

    if (bBuildOnBeginPlay)
    {
        BuildSystemScene();
    }

    bAnimateBodies = false;

    SpawnSkyLight();

    // Do NOT spawn directional light when using custom planet material lighting.
    // DirectionalLight creates a global terminator that fights per-planet LightDirection.
    // SpawnDirectionalLight();
}

void ASystemSceneBuilder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ++TickCounter;
    DebugTickAccumulator += DeltaTime;

    if (bAnimateBodies)
    {
        RefreshRuntimeBodyTransforms();
    }

    const bool bShouldLogThisTick =
        bEnableDebugLogs &&
        (bLogEveryTick || (DebugLogIntervalSeconds > 0.0f && DebugTickAccumulator >= DebugLogIntervalSeconds));

    if (bShouldLogThisTick)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Tick #%d DeltaTime=%.4f TrackedBodies=%d ActorMap=%d SpawnedBodies=%d"),
            TickCounter,
            DeltaTime,
            RuntimeBodyMap.Num(),
            BodyActorMap.Num(),
            SpawnedBodies.Num());

        LogTrackedBodies(TEXT("Tick"));
        DebugTickAccumulator = 0.0f;
    }
}

void ASystemSceneBuilder::BuildSystemScene()
{
    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildSystemScene called: DataSource=%d TargetGalaxy='%s' TargetSystem='%s' ScaleMode=%d"),
            (int32)DataSource,
            *TargetGalaxyName,
            *TargetSystemName,
            (int32)ScaleSettings.ScaleMode);
    }

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SystemSceneBuilder] BuildSystemScene: World is null"));
        return;
    }

    if (bDestroyPreviousBodiesOnBuild)
    {
        ClearSpawnedBodies();
    }

    StarSystem* RuntimeSystem = ResolveRuntimeSystem();
    if (!RuntimeSystem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildSystemScene: no runtime system resolved"));
        return;
    }

    LastResolvedName = ANSI_TO_TCHAR(RuntimeSystem->GetName());

    if (bEnableDebugLogs)
    {
        LogRuntimeSystemSummary(RuntimeSystem);
    }

    BuildFromRuntimeSystem(RuntimeSystem);
}

float ASystemSceneBuilder::ConvertOrbitKmToSceneUnits(float OrbitKm, bool bIsMoonOrbit) const
{
    if (ScaleSettings.ScaleMode == ESystemSceneScaleMode::LegacyCinematic)
    {
        const float UnitsPerKm = bIsMoonOrbit
            ? ScaleSettings.LegacyMoonUnitsPerKm
            : ScaleSettings.LegacyUnitsPerKm;

        return FMath::Min(
            OrbitKm * UnitsPerKm,
            ScaleSettings.LegacyMaxOrbitUnits);
    }

    float Units = 0.0f;

    if (ScaleSettings.bUseLogOrbitScaling)
    {
        Units = USystemUtils::OrbitKmToSceneUnitsLog(
            OrbitKm,
            ScaleSettings.MinOrbitKm,
            ScaleSettings.MaxOrbitKm,
            ScaleSettings.MinOrbitUnits,
            ScaleSettings.MaxOrbitUnits);
    }
    else
    {
        Units = USystemUtils::OrbitKmToSceneUnitsClamped(
            OrbitKm,
            ScaleSettings.OrbitUnitsPerMillionKm,
            ScaleSettings.MinOrbitUnits,
            ScaleSettings.MaxOrbitUnits);
    }

    if (bIsMoonOrbit)
    {
        Units *= ScaleSettings.MoonOrbitMultiplier;
    }

    return Units;
}

void ASystemSceneBuilder::ClearSpawnedBodies()
{
    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ClearSpawnedBodies Actors=%d Bodies=%d Regions=%d RuntimeBodyMap=%d BodyActorMap=%d"),
            SpawnedActors.Num(),
            SpawnedBodies.Num(),
            SpawnedRegions.Num(),
            RuntimeBodyMap.Num(),
            BodyActorMap.Num());
    }

    // ----------------------------------------------------
    // RESET CUTSCENE SCALES FIRST
    // ----------------------------------------------------
    ResetTemporaryCutsceneBodyScales();

    // ----------------------------------------------------
    // Destroy spawned actors
    // ----------------------------------------------------
    for (AActor* Spawned : SpawnedActors)
    {
        if (IsValid(Spawned))
        {
            Spawned->Destroy();
        }
    }

    // ----------------------------------------------------
    // Clear containers
    // ----------------------------------------------------
    SpawnedActors.Empty();
    SpawnedBodies.Empty();
    SpawnedRegions.Empty();
    RuntimeBodyMap.Empty();
    BodyActorMap.Empty();
}

bool ASystemSceneBuilder::ResolveStarSystemRow(FStarSystem& OutRow) const
{
    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveStarSystemRow: Environment subsystem is null"));
        return false;
    }

    const FStarSystem* Found = Env->FindStarSystemByName(TargetSystemName);
    if (!Found)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveStarSystemRow: no match for '%s'"),
            *TargetSystemName);
        return false;
    }

    OutRow = *Found;
    return true;
}

bool ASystemSceneBuilder::ResolveGalaxyRow(FS_Galaxy& OutRow) const
{
    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveGalaxyRow: Environment subsystem is null"));
        return false;
    }

    const FS_Galaxy* Found = Env->FindGalaxyByName(TargetGalaxyName);
    if (!Found)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveGalaxyRow: no match for '%s'"),
            *TargetGalaxyName);
        return false;
    }

    OutRow = *Found;
    return true;
}

StarSystem* ASystemSceneBuilder::ResolveRuntimeSystem() const
{
    FString ResolvedName;

    if (DataSource == ESystemSceneSource::Galaxy)
    {
        ResolvedName = TargetGalaxyName;
    }
    else
    {
        ResolvedName = TargetSystemName;
    }

    if (ResolvedName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveRuntimeSystem: target name is empty"));
        return nullptr;
    }

    StarSystem* RuntimeSystem = StarSystemRegistry::Find(ResolvedName);
    if (!RuntimeSystem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolveRuntimeSystem: runtime system not found for '%s'"),
            *ResolvedName);
        return nullptr;
    }

    return RuntimeSystem;
}

OrbitalBody* ASystemSceneBuilder::ResolvePrimaryStar(StarSystem* RuntimeSystem) const
{
    if (!RuntimeSystem)
    {
        return nullptr;
    }

    List<OrbitalBody>& Bodies = RuntimeSystem->GetBodies();
    ListIter<OrbitalBody> StarIter = Bodies;

    while (++StarIter)
    {
        OrbitalBody* StarBody = StarIter.value();
        if (StarBody)
        {
            return StarBody;
        }
    }

    return nullptr;
}

void ASystemSceneBuilder::BuildFromRuntimeSystem(StarSystem* RuntimeSystem)
{
    if (!RuntimeSystem)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildFromRuntimeSystem: RuntimeSystem is null"));
        return;
    }

    RuntimeSystem->ExecFrame();

    OrbitalBody* PrimaryStar = ResolvePrimaryStar(RuntimeSystem);
    if (!PrimaryStar)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildFromRuntimeSystem: no primary star found in '%s'"),
            ANSI_TO_TCHAR(RuntimeSystem->GetName()));
        return;
    }

    const FVector StarAnchorWorldLocation = GetActorLocation();
    const FVector PrimaryStarRuntimeLocation = PrimaryStar->Location();

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Building runtime system '%s' primary star '%s' anchored at %s RuntimeStarLoc=%s ScaleMode=%d"),
            ANSI_TO_TCHAR(RuntimeSystem->GetName()),
            ANSI_TO_TCHAR(PrimaryStar->GetName()),
            *StarAnchorWorldLocation.ToString(),
            *PrimaryStarRuntimeLocation.ToString(),
            (int32)ScaleSettings.ScaleMode);
    }

    AActor* StarActor = nullptr;
    BuildRuntimeStar(
        PrimaryStar,
        StarAnchorWorldLocation,
        nullptr,
        StarActor);

    ListIter<OrbitalBody> PlanetIter = PrimaryStar->Satellites();
    while (++PlanetIter)
    {
        OrbitalBody* PlanetBody = PlanetIter.value();
        if (!PlanetBody)
        {
            continue;
        }

        BuildRuntimePlanet(
            PlanetBody,
            StarAnchorWorldLocation,
            PrimaryStarRuntimeLocation,
            StarActor);
    }

    if (bEnableDebugLogs)
    {
        LogTrackedBodies(TEXT("PostBuild"));
    }

    if (ScaleSettings.ScaleMode == ESystemSceneScaleMode::PreviewCompressed)
    {
        FocusPlayerCameraOnSpawnedBodies();
    }
}

void ASystemSceneBuilder::BuildRuntimeStar(
    OrbitalBody* StarBody,
    const FVector& StarAnchorWorldLocation,
    AActor* ParentActor,
    AActor*& OutStarActor)
{
    OutStarActor = nullptr;

    if (!StarBody)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeStar: StarBody is null"));
        return;
    }

    const FString StarName = ANSI_TO_TCHAR(StarBody->GetName());
    const float RadiusUnits = ConvertStarRadiusKmToSceneUnits(
        StarBody->Radius() > 0.0 ? (float)StarBody->Radius() : 696340.0f);

    AActor* StarActor = SpawnBodyActor(
        StarActorClass,
        StarName,
        StarAnchorWorldLocation,
        RadiusUnits,
        ParentActor,
        false,
        true);

    OutStarActor = StarActor;

    RegisterSpawnedBody(
        StarName,
        StarActor,
        ParentActor,
        false,
        false,
        StarAnchorWorldLocation,
        RadiusUnits,
        0.0f);

    TrackRuntimeBody(StarName, StarBody, StarActor);

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] STAR '%s' RuntimeLoc=%s SceneLoc=%s RadiusKm=%.2f RadiusUnits=%.2f Actor=%s"),
            *StarName,
            *StarBody->Location().ToString(),
            *StarAnchorWorldLocation.ToString(),
            StarBody->Radius(),
            RadiusUnits,
            *GetNameSafe(StarActor));
    }
}

void ASystemSceneBuilder::BuildRuntimePlanet(
    OrbitalBody* PlanetBody,
    const FVector& StarAnchorWorldLocation,
    const FVector& PrimaryStarRuntimeLocation,
    AActor* ParentActor)
{
    if (!PlanetBody)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimePlanet: PlanetBody is null"));
        return;
    }

    const FString PlanetName = ANSI_TO_TCHAR(PlanetBody->GetName());

    FVector RuntimeOffsetKm =
        PlanetBody->Location() - PrimaryStarRuntimeLocation;

    if (ScaleSettings.bRuntimeLocationsAreMeters)
    {
        RuntimeOffsetKm /= 1000.0f;
    }

    const FVector SceneOffset =
        ConvertRuntimeOffsetToSceneOffset(RuntimeOffsetKm, false);

    const FVector PlanetWorldLocation =
        StarAnchorWorldLocation + SceneOffset;

    const EPlanetType PlanetType =
        ResolvePlanetTypeFromData(PlanetName, false);

    const FPlanet* PlanetData = nullptr;

    if (UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem())
    {
        PlanetData = Env->FindPlanetMapByName(PlanetName);
    }

    float PlanetRadiusMeters =
        PlanetBody->Radius() > 0.0f
        ? static_cast<float>(PlanetBody->Radius())
        : 2000000.0f;

    if (PlanetData && PlanetData->Radius > 0.0)
    {
        PlanetRadiusMeters = static_cast<float>(PlanetData->Radius);
    }

    float RadiusUnits =
        ConvertPlanetRadiusToSceneUnits(PlanetRadiusMeters);

    if (PlanetType == EPlanetType::GasGiant)
    {
        RadiusUnits = FMath::Clamp(
            RadiusUnits,
            ScaleSettings.MinGasGiantScaleUnits,
            ScaleSettings.MaxGasGiantScaleUnits);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[PlanetRadius] %s Type=%d RadiusMeters=%.2f RadiusUnits=%.2f"),
        *PlanetName,
        (int32)PlanetType,
        PlanetRadiusMeters,
        RadiusUnits);

    const float OrbitRadiusUnits =
        ConvertOrbitKmToSceneUnits((float)RuntimeOffsetKm.Size(), false);

    if (bSpawnOrbitActors && OrbitActorClass && OrbitRadiusUnits > 0.0f)
    {
        SpawnOrbitActor(
            PlanetName,
            StarAnchorWorldLocation,
            OrbitRadiusUnits,
            ParentActor);
    }

    TSubclassOf<AActor> ResolvedClass = nullptr;

    if (PlanetType == EPlanetType::GasGiant && GasGiantPlanetActorClass)
    {
        ResolvedClass = GasGiantPlanetActorClass;
    }
    else
    {
        ResolvedClass = ResolvePlanetActorClass(PlanetType, false);
    }

    AActor* PlanetActor = SpawnBodyActor(
        ResolvedClass,
        PlanetName,
        PlanetWorldLocation,
        RadiusUnits,
        ParentActor,
        false,
        false);

    if (!PlanetActor)
    {
        return;
    }

    const FVector RawLightDir =
        (StarAnchorWorldLocation - PlanetWorldLocation).GetSafeNormal();

    const FVector CorrectedLightDir(
        -RawLightDir.Z,
        RawLightDir.Y,
        RawLightDir.X);

    if (AGasGiantActor* Gas = Cast<AGasGiantActor>(PlanetActor))
    {
        Gas->SetPlanetRadius(RadiusUnits);
        Gas->SetLightDirection(CorrectedLightDir);

        if (PlanetData)
        {
            Gas->SetGasGiantMaterialByName(PlanetData->Texture);

            const bool bHasRing =
                !PlanetData->Ring.TrimStartAndEnd().IsEmpty() &&
                PlanetData->Minrad > 0.0 &&
                PlanetData->Maxrad > PlanetData->Minrad &&
                PlanetData->Radius > 0.0;

            if (bHasRing)
            {
                const float InnerUnits =
                    RadiusUnits * static_cast<float>(PlanetData->Minrad / PlanetData->Radius);

                const float OuterUnits =
                    RadiusUnits * static_cast<float>(PlanetData->Maxrad / PlanetData->Radius);

                Gas->InnerRingRadius = InnerUnits;
                Gas->OuterRingRadius = OuterUnits;
                Gas->RingPosition = 0.0f;
                Gas->ApplyRingRadiusSettings();

                UE_LOG(LogTemp, Warning,
                    TEXT("[SystemSceneBuilder] GasGiant Ring '%s' Ring='%s' Minrad=%.2f Maxrad=%.2f InnerUnits=%.2f OuterUnits=%.2f"),
                    *PlanetName,
                    *PlanetData->Ring,
                    PlanetData->Minrad,
                    PlanetData->Maxrad,
                    InnerUnits,
                    OuterUnits);
            }
            else
            {
                Gas->InnerRingRadius = 0.0f;
                Gas->OuterRingRadius = 0.0f;
                Gas->RingPosition = 0.0f;
                Gas->ApplyRingRadiusSettings();

                UE_LOG(LogTemp, Warning,
                    TEXT("[SystemSceneBuilder] GasGiant Ring Disabled '%s' Ring='%s' Minrad=%.2f Maxrad=%.2f Radius=%.2f"),
                    *PlanetName,
                    *PlanetData->Ring,
                    PlanetData->Minrad,
                    PlanetData->Maxrad,
                    PlanetData->Radius);
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] GasGiant '%s' TextureSelector='%s'"),
                *PlanetName,
                *PlanetData->Texture);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GasGiant '%s' LightDir=%s"),
            *PlanetName,
            *CorrectedLightDir.ToString());
    }
    else if (APlanetActor* PlanetVisual = Cast<APlanetActor>(PlanetActor))
    {
        PlanetVisual->SetLightDirection(CorrectedLightDir);

        if (PlanetData)
        {
            auto LoadTex = [](const FString& Name) -> UTexture2D*
                {
                    if (Name.IsEmpty())
                    {
                        return nullptr;
                    }

                    const FString Path = FString::Printf(
                        TEXT("/Game/GameData/Galaxy/PlanetMaterials/%s.%s"),
                        *Name,
                        *Name);

                    return LoadObject<UTexture2D>(nullptr, *Path);
                };

            PlanetVisual->SetPlanetTextures(
                LoadTex(PlanetData->Texture),
                LoadTex(PlanetData->Gloss),
                LoadTex(PlanetData->Lights));
        }
    }

    RegisterSpawnedBody(
        PlanetName,
        PlanetActor,
        ParentActor,
        false,
        false,
        PlanetWorldLocation,
        RadiusUnits,
        OrbitRadiusUnits);

    TrackRuntimeBody(
        PlanetName,
        PlanetBody,
        PlanetActor);

    if (bSpawnRegionActors)
    {
        BuildRuntimeRegionForBody(
            PlanetName,
            PlanetWorldLocation,
            PlanetActor,
            RadiusUnits,
            false);
    }

    ListIter<OrbitalBody> MoonIter =
        PlanetBody->Satellites();

    while (++MoonIter)
    {
        OrbitalBody* MoonBody = MoonIter.value();

        if (!MoonBody)
        {
            continue;
        }

        BuildRuntimeMoon(
            MoonBody,
            PlanetBody,
            StarAnchorWorldLocation,
            PrimaryStarRuntimeLocation,
            PlanetActor);
    }
}

void ASystemSceneBuilder::BuildRuntimeMoon(
    OrbitalBody* MoonBody,
    OrbitalBody* ParentPlanetBody,
    const FVector& StarAnchorWorldLocation,
    const FVector& PrimaryStarRuntimeLocation,
    AActor* ParentActor)
{
    if (!MoonBody)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeMoon: MoonBody is null"));
        return;
    }

    if (!ParentPlanetBody)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeMoon: ParentPlanetBody is null for moon '%s'"),
            ANSI_TO_TCHAR(MoonBody->GetName()));
        return;
    }

    const FString MoonName = ANSI_TO_TCHAR(MoonBody->GetName());

    const FVector ParentWorldLocation =
        ParentActor ? ParentActor->GetActorLocation() : StarAnchorWorldLocation;

    FVector RuntimeOffsetKm =
        MoonBody->Location() - ParentPlanetBody->Location();

    if (ScaleSettings.bRuntimeLocationsAreMeters)
    {
        RuntimeOffsetKm /= 1000.0f;
    }

    const FVector SceneOffset =
        ConvertRuntimeOffsetToSceneOffset(RuntimeOffsetKm, true);

    const FVector MoonWorldLocation =
        ParentWorldLocation + SceneOffset;

    float MoonRadiusMeters =
        MoonBody->Radius() > 0.0f
        ? static_cast<float>(MoonBody->Radius())
        : 500000.0f;

    if (MoonRadiusMeters > 0.0f && MoonRadiusMeters < 100000.0f)
    {
        MoonRadiusMeters *= 1000.0f;
    }

    const float RawRadiusUnits =
        ConvertPlanetRadiusToSceneUnits(MoonRadiusMeters);

    const float RadiusUnits =
        FMath::Clamp(
            RawRadiusUnits,
            ScaleSettings.MinMoonScaleUnits,
            ScaleSettings.MaxMoonScaleUnits);

    const float OrbitRadiusUnits =
        ConvertOrbitKmToSceneUnits(RuntimeOffsetKm.Size(), true);

    if (bSpawnOrbitActors && OrbitActorClass && OrbitRadiusUnits > 0.0f)
    {
        SpawnOrbitActor(
            MoonName,
            ParentWorldLocation,
            OrbitRadiusUnits,
            ParentActor);
    }

    const EPlanetType PlanetType =
        ResolvePlanetTypeFromData(MoonName, true);

    const TSubclassOf<AActor> ResolvedMoonClass =
        ResolvePlanetActorClass(PlanetType, true);

    AActor* MoonActor = SpawnBodyActor(
        ResolvedMoonClass,
        MoonName,
        MoonWorldLocation,
        RadiusUnits,
        ParentActor,
        true,
        false);

    RegisterSpawnedBody(
        MoonName,
        MoonActor,
        ParentActor,
        true,
        false,
        MoonWorldLocation,
        RadiusUnits,
        OrbitRadiusUnits);

    TrackRuntimeBody(
        MoonName,
        MoonBody,
        MoonActor);

    if (bSpawnRegionActors)
    {
        BuildRuntimeRegionForBody(
            MoonName,
            MoonWorldLocation,
            MoonActor,
            RadiusUnits,
            true);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] MOON '%s' Parent='%s' ParentWorld=%s RuntimeOffsetKm=%s SceneOffset=%s SceneLoc=%s RadiusMeters=%.2f RadiusUnits=%.2f OrbitUnits=%.2f Actor=%s"),
        *MoonName,
        ANSI_TO_TCHAR(ParentPlanetBody->GetName()),
        *ParentWorldLocation.ToString(),
        *RuntimeOffsetKm.ToString(),
        *SceneOffset.ToString(),
        *MoonWorldLocation.ToString(),
        MoonRadiusMeters,
        RadiusUnits,
        OrbitRadiusUnits,
        *GetNameSafe(MoonActor));
}

void ASystemSceneBuilder::TrackRuntimeBody(
    const FString& BodyName,
    OrbitalBody* Body,
    AActor* Actor)
{
    if (!BodyName.IsEmpty() && Body)
    {
        RuntimeBodyMap.Add(BodyName, Body);
    }

    if (!BodyName.IsEmpty() && Actor)
    {
        BodyActorMap.Add(BodyName, Actor);
    }

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] TrackRuntimeBody Name='%s' Body=%p Actor=%s RuntimeBodyMap=%d BodyActorMap=%d"),
            *BodyName,
            Body,
            *GetNameSafe(Actor),
            RuntimeBodyMap.Num(),
            BodyActorMap.Num());
    }
}

void ASystemSceneBuilder::RefreshRuntimeBodyTransforms()
{
    if (RuntimeBodyMap.Num() == 0 || BodyActorMap.Num() == 0)
    {
        return;
    }

    OrbitalBody* PrimaryStar = nullptr;

    for (const TPair<FString, OrbitalBody*>& Pair : RuntimeBodyMap)
    {
        if (Pair.Value && Pair.Value->GetType() == Orbital::STAR)
        {
            PrimaryStar = Pair.Value;
            break;
        }
    }

    if (!PrimaryStar)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] RefreshRuntimeBodyTransforms: no primary star found"));
        return;
    }

    const FVector StarRuntimeLoc = PrimaryStar->Location();
    const FVector StarSceneLoc = GetActorLocation();

    int32 MovedCount = 0;

    for (const TPair<FString, OrbitalBody*>& Pair : RuntimeBodyMap)
    {
        const FString& BodyName = Pair.Key;
        OrbitalBody* Body = Pair.Value;
        TObjectPtr<AActor>* FoundActor = BodyActorMap.Find(BodyName);

        if (!Body || !FoundActor || !(*FoundActor))
        {
            continue;
        }

        AActor* Actor = *FoundActor;
        FVector NewLocation = StarSceneLoc;

        if (Body != PrimaryStar)
        {
            const bool bIsMoon = (Body->GetType() == Orbital::MOON);
            const FVector RuntimeOffset = Body->Location() - StarRuntimeLoc;
            const FVector SceneOffset = ConvertRuntimeOffsetToSceneOffset(RuntimeOffset, bIsMoon);
            NewLocation = StarSceneLoc + SceneOffset;
        }

        const FVector OldLocation = Actor->GetActorLocation();
        const bool bMoved = !OldLocation.Equals(NewLocation, 0.01f);

        const FVector SmoothedLocation = FMath::VInterpTo(
            OldLocation,
            NewLocation,
            GetWorld()->GetDeltaSeconds(),
            3.0f);

        Actor->SetActorLocation(SmoothedLocation);

        for (FSpawnedSystemBody& Entry : SpawnedBodies)
        {
            if (Entry.Actor == Actor)
            {
                Entry.SpawnLocation = NewLocation;
                break;
            }
        }

        if (bMoved)
        {
            ++MovedCount;
        }

        if (bEnableDebugLogs && bLogEveryTick)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] Refresh Body='%s' Type=%d RuntimeLoc=%s StarRuntimeLoc=%s OldLoc=%s NewLoc=%s Moved=%s"),
                *BodyName,
                (int32)Body->GetType(),
                *Body->Location().ToString(),
                *StarRuntimeLoc.ToString(),
                *OldLocation.ToString(),
                *NewLocation.ToString(),
                bMoved ? TEXT("true") : TEXT("false"));
        }
    }

    if (bEnableDebugLogs && !bLogEveryTick && DebugLogIntervalSeconds > 0.0f && DebugTickAccumulator >= DebugLogIntervalSeconds)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] RefreshRuntimeBodyTransforms complete. MovedCount=%d"),
            MovedCount);
    }
}

AActor* ASystemSceneBuilder::SpawnBodyActor(
    TSubclassOf<AActor> BodyClass,
    const FString& BodyName,
    const FVector& WorldLocation,
    float VisualRadiusUnits,
    AActor* ParentActor,
    bool bIsMoon,
    bool bIsStar)
{
    if (!BodyClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnBodyActor: BodyClass is null for '%s'"),
            *BodyName);
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnBodyActor: World is null for '%s'"),
            *BodyName);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Spawned = World->SpawnActor<AActor>(
        BodyClass,
        WorldLocation,
        FRotator::ZeroRotator,
        Params);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnBodyActor: FAILED to spawn '%s'"),
            *BodyName);
        return nullptr;
    }

#if WITH_EDITOR
    Spawned->SetActorLabel(BodyName);
#endif

    if (ParentActor)
    {
        Spawned->AttachToActor(
            ParentActor,
            FAttachmentTransformRules::KeepWorldTransform);
    }

    const float SafeRadius = FMath::Max(VisualRadiusUnits, 1.0f);

    if (AGasGiantActor* GasGiant = Cast<AGasGiantActor>(Spawned))
    {
        GasGiant->SetPlanetRadius(SafeRadius);

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GasGiantActor '%s' RadiusUnits=%.2f"),
            *BodyName,
            SafeRadius);
    }
    else if (APlanetActor* Planet = Cast<APlanetActor>(Spawned))
    {
        Planet->SetPlanetRadius(SafeRadius);

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] PlanetActor '%s' RadiusUnits=%.2f"),
            *BodyName,
            SafeRadius);
    }
    else
    {
        const float FinalScale = FMath::Clamp(
            VisualRadiusUnits,
            ScaleSettings.MinBodyScaleUnits,
            ScaleSettings.MaxBodyScaleUnits);

        Spawned->SetActorScale3D(FVector(FinalScale));

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GenericActor '%s' Scale=%.2f"),
            *BodyName,
            FinalScale);
    }

    if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Spawned))
    {
        if (UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent())
        {
            SMC->SetMobility(EComponentMobility::Movable);

            if (Spawned->GetClass() == AStaticMeshActor::StaticClass())
            {
                static UStaticMesh* SphereMesh = nullptr;

                if (!SphereMesh)
                {
                    SphereMesh = LoadObject<UStaticMesh>(
                        nullptr,
                        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
                }

                if (SphereMesh)
                {
                    SMC->SetStaticMesh(SphereMesh);
                }
            }
        }
    }

    SpawnedActors.Add(Spawned);
    BodyActorMap.Add(BodyName, Spawned);

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] SpawnBodyActor SUCCESS '%s' Actor=%s Class=%s Loc=%s RadiusUnits=%.2f Parent=%s"),
        *BodyName,
        *GetNameSafe(Spawned),
        *GetNameSafe(Spawned->GetClass()),
        *WorldLocation.ToString(),
        VisualRadiusUnits,
        *GetNameSafe(ParentActor));

    return Spawned;
}

AActor* ASystemSceneBuilder::SpawnOrbitActor(
    const FString& OrbitName,
    const FVector& WorldLocation,
    float OrbitRadiusUnits,
    AActor* ParentActor)
{
    if (!OrbitActorClass)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnOrbitActor: OrbitActorClass is null for '%s'"),
            *OrbitName);
        return nullptr;
    }

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnOrbitActor: World is null for '%s'"),
            *OrbitName);
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const FString FullName = OrbitName + TEXT("_ORBIT");
    SpawnParams.Name = FName(*FullName);

    AActor* Spawned = GetWorld()->SpawnActor<AActor>(
        OrbitActorClass,
        WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnOrbitActor: failed to spawn '%s'"),
            *OrbitName);
        return nullptr;
    }

#if WITH_EDITOR
    Spawned->SetActorLabel(FullName);
#endif

    USceneComponent* Root = Spawned->GetRootComponent();
    if (Root)
    {
        Root->SetMobility(EComponentMobility::Movable);
    }

    AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Spawned);
    if (SMA && SMA->GetStaticMeshComponent())
    {
        SMA->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);

        static UStaticMesh* SphereMesh = nullptr;
        if (!SphereMesh)
        {
            SphereMesh = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        }

        if (SphereMesh)
        {
            SMA->GetStaticMeshComponent()->SetStaticMesh(SphereMesh);
        }
    }

    Spawned->SetActorScale3D(FVector(OrbitRadiusUnits));

    if (ParentActor)
    {
        Spawned->AttachToActor(
            ParentActor,
            FAttachmentTransformRules::KeepWorldTransform);
    }
    else
    {
        Spawned->AttachToActor(
            this,
            FAttachmentTransformRules::KeepWorldTransform);
    }

    SpawnedActors.Add(Spawned);

    RegisterSpawnedBody(
        FullName,
        Spawned,
        ParentActor,
        false,
        true,
        WorldLocation,
        0.0f,
        OrbitRadiusUnits);

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Spawned orbit '%s' Actor=%s Loc=%s Radius=%.2f"),
            *FullName,
            *GetNameSafe(Spawned),
            *WorldLocation.ToString(),
            OrbitRadiusUnits);
    }

    return Spawned;
}

float ASystemSceneBuilder::ConvertRadiusKmToSceneUnits(float RadiusKm) const
{
    return USystemUtils::RadiusKmToSceneUnitsClamped(
        RadiusKm,
        ScaleSettings.RadiusUnitsPerThousandKm,
        ScaleSettings.MinBodyScaleUnits,
        ScaleSettings.MaxBodyScaleUnits);
}

FVector ASystemSceneBuilder::ConvertRuntimeOffsetToSceneOffset(
    const FVector& RuntimeOffsetKm,
    bool bIsMoon) const
{
    const float RuntimeDistanceKm = RuntimeOffsetKm.Size();
    const float SceneDistance = ConvertOrbitKmToSceneUnits(RuntimeDistanceKm, bIsMoon);

    FVector Direction = RuntimeOffsetKm.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        Direction = FVector::ForwardVector;
    }

    const FVector SceneOffset(
        Direction.X * SceneDistance,
        ScaleSettings.VerticalOffset,
        Direction.Y * SceneDistance);

    if (bEnableDebugLogs && bLogEveryTick)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ConvertRuntimeOffsetToSceneOffset RuntimeOffset=%s DistanceKm=%.2f SceneDistance=%.2f Dir=%s SceneOffset=%s bIsMoon=%s"),
            *RuntimeOffsetKm.ToString(),
            RuntimeDistanceKm,
            SceneDistance,
            *Direction.ToString(),
            *SceneOffset.ToString(),
            bIsMoon ? TEXT("true") : TEXT("false"));
    }

    return SceneOffset;
}

FVector ASystemSceneBuilder::ConvertLegacyCameraOffsetToSceneOffset(const FVector& LegacyOffset) const
{
    if (ScaleSettings.ScaleMode == ESystemSceneScaleMode::LegacyCinematic)
    {
        return FVector(
            LegacyOffset.X,
            LegacyOffset.Z,
            LegacyOffset.Y);
    }

    const float LegacyDistance = LegacyOffset.Size();

    if (LegacyDistance <= KINDA_SMALL_NUMBER)
    {
        return FVector(-2500.0f, -500.0f, 1500.0f);
    }

    const float SceneDistance = ConvertOrbitKmToSceneUnits(LegacyDistance, false);

    FVector Dir = LegacyOffset.GetSafeNormal();
    if (Dir.IsNearlyZero())
    {
        Dir = FVector(0.0f, 0.0f, 1.0f);
    }

    const FVector SceneOffset(
        Dir.X * SceneDistance,
        Dir.Z * SceneDistance,
        Dir.Y * SceneDistance);

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] ConvertLegacyCameraOffsetToSceneOffset LegacyOffset=%s LegacyDistance=%.2f SceneDistance=%.2f Dir=%s SceneOffset=%s"),
        *LegacyOffset.ToString(),
        LegacyDistance,
        SceneDistance,
        *Dir.ToString(),
        *SceneOffset.ToString());

    return SceneOffset;
}

UStarshatterEnvironmentSubsystem* ASystemSceneBuilder::GetEnvironmentSubsystem() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();
}

void ASystemSceneBuilder::FocusPlayerCameraOnSystem(const FVector& FocusPoint, float Distance) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    const FVector CameraLocation = FocusPoint + FVector(-Distance, 0.0f, Distance * 0.35f);
    const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

    if (PC->PlayerCameraManager)
    {
        PC->SetInitialLocationAndRotation(CameraLocation, CameraRotation);
        PC->SetControlRotation(CameraRotation);
    }

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Camera moved to %s looking at %s"),
            *CameraLocation.ToString(),
            *FocusPoint.ToString());
    }
}

void ASystemSceneBuilder::FocusPlayerCameraOnSpawnedBodies() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    bool bHasAnyBodies = false;
    FVector MinBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    FVector MaxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    float MaxStarRadius = 0.0f;

    for (const FSpawnedSystemBody& Entry : SpawnedBodies)
    {
        if (Entry.bIsOrbit || !Entry.Actor)
        {
            continue;
        }

        bHasAnyBodies = true;

        const FVector P = Entry.SpawnLocation;

        MinBounds.X = FMath::Min(MinBounds.X, P.X);
        MinBounds.Y = FMath::Min(MinBounds.Y, P.Y);
        MinBounds.Z = FMath::Min(MinBounds.Z, P.Z);

        MaxBounds.X = FMath::Max(MaxBounds.X, P.X);
        MaxBounds.Y = FMath::Max(MaxBounds.Y, P.Y);
        MaxBounds.Z = FMath::Max(MaxBounds.Z, P.Z);

        if (!Entry.bIsMoon && !Entry.ParentActor)
        {
            MaxStarRadius = FMath::Max(MaxStarRadius, Entry.VisualRadiusUnits);
        }
    }

    if (!bHasAnyBodies)
    {
        return;
    }

    const FVector FocusPoint = (MinBounds + MaxBounds) * 0.5f;

    const float SpanX = MaxBounds.X - MinBounds.X;
    const float SpanY = MaxBounds.Y - MinBounds.Y;
    const float SpanZ = MaxBounds.Z - MinBounds.Z;
    const float MaxSpan = FMath::Max3(SpanX, SpanY, SpanZ);

    const float Distance = FMath::Max(MaxSpan * 2.0f, MaxStarRadius * 6.0f);
    const FVector CameraLocation = FocusPoint + FVector(-Distance, -Distance * 0.25f, Distance * 0.85f);
    const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

    PC->SetInitialLocationAndRotation(CameraLocation, CameraRotation);
    PC->SetControlRotation(CameraRotation);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Camera Focus=%s Camera=%s SpanX=%.2f SpanY=%.2f SpanZ=%.2f MaxStarRadius=%.2f"),
            *FocusPoint.ToString(),
            *CameraLocation.ToString(),
            SpanX,
            SpanY,
            SpanZ,
            MaxStarRadius);
    }
}

float ASystemSceneBuilder::ConvertStarRadiusKmToSceneUnits(float RadiusKm) const
{
    const float Raw = USystemUtils::RadiusKmToSceneUnitsClamped(
        RadiusKm,
        ScaleSettings.RadiusUnitsPerThousandKm,
        ScaleSettings.MinBodyScaleUnits,
        ScaleSettings.MaxBodyScaleUnits);

    return FMath::Clamp(
        Raw,
        ScaleSettings.MinStarScaleUnits,
        ScaleSettings.MaxStarScaleUnits);
}

float ASystemSceneBuilder::ConvertPlanetRadiusToSceneUnits(float RadiusMeters) const
{
    const float RadiusKm = RadiusMeters / 1000.0f;

    const float Units =
        (RadiusKm / 1000.0f) *
        ScaleSettings.RadiusUnitsPerThousandKm;

    return FMath::Clamp(
        Units,
        ScaleSettings.MinPlanetScaleUnits,
        ScaleSettings.MaxPlanetScaleUnits);
}

void ASystemSceneBuilder::LogRuntimeSystemSummary(StarSystem* RuntimeSystem) const
{
    if (!RuntimeSystem)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] RuntimeSystem Summary Name='%s' Location=%s Radius=%.2f"),
        ANSI_TO_TCHAR(RuntimeSystem->GetName()),
        *RuntimeSystem->GetLocation().ToString(),
        RuntimeSystem->GetRadius());

    List<OrbitalBody>& Bodies = RuntimeSystem->GetBodies();
    ListIter<OrbitalBody> StarIter = Bodies;

    while (++StarIter)
    {
        OrbitalBody* StarBody = StarIter.value();
        if (!StarBody)
        {
            continue;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder]   STAR Name='%s' Loc=%s Radius=%.2f Orbit=%.2f"),
            ANSI_TO_TCHAR(StarBody->GetName()),
            *StarBody->Location().ToString(),
            StarBody->Radius(),
            StarBody->Orbit());

        ListIter<OrbitalBody> PlanetIter = StarBody->Satellites();
        while (++PlanetIter)
        {
            OrbitalBody* PlanetBody = PlanetIter.value();
            if (!PlanetBody)
            {
                continue;
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder]     PLANET Name='%s' Loc=%s Radius=%.2f Orbit=%.2f"),
                ANSI_TO_TCHAR(PlanetBody->GetName()),
                *PlanetBody->Location().ToString(),
                PlanetBody->Radius(),
                PlanetBody->Orbit());

            ListIter<OrbitalBody> MoonIter = PlanetBody->Satellites();
            while (++MoonIter)
            {
                OrbitalBody* MoonBody = MoonIter.value();
                if (!MoonBody)
                {
                    continue;
                }

                UE_LOG(LogTemp, Warning,
                    TEXT("[SystemSceneBuilder]       MOON Name='%s' Loc=%s Radius=%.2f Orbit=%.2f"),
                    ANSI_TO_TCHAR(MoonBody->GetName()),
                    *MoonBody->Location().ToString(),
                    MoonBody->Radius(),
                    MoonBody->Orbit());
            }
        }
    }
}

void ASystemSceneBuilder::LogTrackedBodies(const TCHAR* Label) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] %s RuntimeBodyMap=%d BodyActorMap=%d"),
        Label ? Label : TEXT("TrackedBodies"),
        RuntimeBodyMap.Num(),
        BodyActorMap.Num());

    for (const TPair<FString, OrbitalBody*>& Pair : RuntimeBodyMap)
    {
        const FString& Name = Pair.Key;
        OrbitalBody* Body = Pair.Value;
        const TObjectPtr<AActor>* FoundActor = BodyActorMap.Find(Name);

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder]   %s Body=%p Type=%d RuntimeLoc=%s Actor=%s ActorLoc=%s"),
            *Name,
            Body,
            Body ? (int32)Body->GetType() : -1,
            Body ? *Body->Location().ToString() : TEXT("<null>"),
            FoundActor && *FoundActor ? *GetNameSafe(*FoundActor) : TEXT("<null>"),
            FoundActor && *FoundActor ? *(*FoundActor)->GetActorLocation().ToString() : TEXT("<null>"));
    }
}

bool ASystemSceneBuilder::FindSpawnedBodyByName(
    const FString& BodyName,
    FSpawnedSystemBody& OutBody) const
{
    const FString SearchName = BodyName.TrimStartAndEnd();

    if (SearchName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FindSpawnedBodyByName: empty search name"));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[FindBody] Searching='%s' SpawnedBodies=%d"),
        *SearchName,
        SpawnedBodies.Num());

    const FSpawnedSystemBody* ExactBodyMatch = nullptr;
    const FSpawnedSystemBody* PartialBodyMatch = nullptr;

    for (const FSpawnedSystemBody& Entry : SpawnedBodies)
    {
        const FString EntryName = Entry.BodyName.TrimStartAndEnd();

        UE_LOG(LogTemp, Warning,
            TEXT("[FindBody] Candidate BodyName='%s' Actor=%s IsOrbit=%d IsMoon=%d Parent=%s Radius=%.2f"),
            *EntryName,
            *GetNameSafe(Entry.Actor),
            Entry.bIsOrbit ? 1 : 0,
            Entry.bIsMoon ? 1 : 0,
            *GetNameSafe(Entry.ParentActor),
            Entry.VisualRadiusUnits);

        if (Entry.bIsOrbit)
        {
            continue;
        }

        if (EntryName.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            ExactBodyMatch = &Entry;
            break;
        }

        if (!PartialBodyMatch &&
            EntryName.Contains(SearchName, ESearchCase::IgnoreCase))
        {
            PartialBodyMatch = &Entry;
        }
    }

    const FSpawnedSystemBody* BestMatch =
        ExactBodyMatch ? ExactBodyMatch : PartialBodyMatch;

    if (!BestMatch)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[FindBody] FAILED Search='%s'. No body match."),
            *SearchName);
        return false;
    }

    OutBody = *BestMatch;

    UE_LOG(LogTemp, Warning,
        TEXT("[FindBody] FOUND Search='%s' -> BodyName='%s' Actor=%s Loc=%s Radius=%.2f"),
        *SearchName,
        *OutBody.BodyName,
        *GetNameSafe(OutBody.Actor),
        OutBody.Actor ? *OutBody.Actor->GetActorLocation().ToString() : TEXT("NULL"),
        OutBody.VisualRadiusUnits);

    return true;
}

bool ASystemSceneBuilder::GetBodyWorldLocationByName(
    const FString& BodyName,
    FVector& OutWorldLocation) const
{
    OutWorldLocation = FVector::ZeroVector;

    FSpawnedSystemBody FoundBody;
    if (!FindSpawnedBodyByName(BodyName, FoundBody))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GetBodyWorldLocationByName: body not found '%s'"),
            *BodyName);
        return false;
    }

    if (!FoundBody.Actor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GetBodyWorldLocationByName: found '%s' but actor is null"),
            *BodyName);
        return false;
    }

    OutWorldLocation = FoundBody.Actor->GetActorLocation();

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] GetBodyWorldLocationByName: '%s' -> %s"),
        *BodyName,
        *OutWorldLocation.ToString());

    return true;
}

bool ASystemSceneBuilder::FocusCameraOnBodyByName(
    const FString& BodyName,
    const FVector& CameraOffset,
    float BlendSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: World is null"));
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: PlayerController is null"));
        return false;
    }

    UE_LOG(LogTemp, Error,
        TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: Body='%s' CameraOffset=%s Blend=%.2f"),
        *BodyName,
        *CameraOffset.ToString(),
        BlendSeconds);

    FSpawnedSystemBody FoundBody;
    if (!FindSpawnedBodyByName(BodyName, FoundBody))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: body not found '%s'"),
            *BodyName);
        return false;
    }

    UE_LOG(LogTemp, Error,
        TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: Target='%s' Found='%s' Actor=%s Parent=%s IsOrbit=%d IsMoon=%d Radius=%.2f Loc=%s"),
        *BodyName,
        *FoundBody.BodyName,
        *GetNameSafe(FoundBody.Actor),
        *GetNameSafe(FoundBody.ParentActor),
        FoundBody.bIsOrbit ? 1 : 0,
        FoundBody.bIsMoon ? 1 : 0,
        FoundBody.VisualRadiusUnits,
        FoundBody.Actor ? *FoundBody.Actor->GetActorLocation().ToString() : TEXT("NULL"));

    if (!FoundBody.Actor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: found '%s' but Actor is null"),
            *BodyName);
        return false;
    }

    const FVector FocusPoint = FoundBody.Actor->GetActorLocation();

    const bool bIsPlanetOrMoon =
        !FoundBody.bIsOrbit &&
        (FoundBody.bIsMoon || FoundBody.ParentActor != nullptr);

    FVector FinalOffset = FVector::ZeroVector;

    if (bIsPlanetOrMoon)
    {
        const float VisualRadius =
            FMath::Max(FoundBody.VisualRadiusUnits, 100.0f);

        const float CameraDistance = FMath::Clamp(
            VisualRadius * 6.0f,
            2000.0f,
            30000.0f);

        if (!CameraOffset.IsNearlyZero())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName Ignoring legacy CameraOffset for planet '%s': %s"),
                *BodyName,
                *CameraOffset.ToString());
        }

        const FVector Dir =
            FVector(-1.0f, -0.25f, 0.45f).GetSafeNormal();

        FinalOffset = Dir * CameraDistance;

        UE_LOG(LogTemp, Warning,
            TEXT("[PlanetCameraDistance FIXED] Target='%s' VisualRadius=%.2f CameraDistance=%.2f Dir=%s FinalOffset=%s"),
            *BodyName,
            VisualRadius,
            CameraDistance,
            *Dir.ToString(),
            *FinalOffset.ToString());
    }
    else
    {
        FinalOffset = ConvertLegacyCameraOffsetToSceneOffset(CameraOffset);

        if (FinalOffset.IsNearlyZero())
        {
            FinalOffset = FVector(-2500.0f, -500.0f, 1500.0f);
        }
    }

    const FVector CameraLocation = FocusPoint + FinalOffset;
    const FRotator CameraRotation =
        (FocusPoint - CameraLocation).Rotation();

    PC->SetInitialLocationAndRotation(CameraLocation, CameraRotation);
    PC->SetControlRotation(CameraRotation);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CameraFinal] Target='%s' PlanetOrMoon=%s Focus=%s Camera=%s Offset=%s Dist=%.2f Radius=%.2f Blend=%.2f"),
        *BodyName,
        bIsPlanetOrMoon ? TEXT("true") : TEXT("false"),
        *FocusPoint.ToString(),
        *CameraLocation.ToString(),
        *FinalOffset.ToString(),
        FinalOffset.Size(),
        FoundBody.VisualRadiusUnits,
        BlendSeconds);

    if (PC->PlayerCameraManager)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[FOCUS CAMERA ACTUAL] Body='%s' CameraManagerLoc=%s CameraManagerRot=%s"),
            *BodyName,
            *PC->PlayerCameraManager->GetCameraLocation().ToString(),
            *PC->PlayerCameraManager->GetCameraRotation().ToString());
    }
    return true;
}

bool ASystemSceneBuilder::ApplyCameraViewVector(
    const FVector& ViewVector,
    float BlendSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ApplyCameraViewVector: World is null"));
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ApplyCameraViewVector: PlayerController or CameraManager is null"));
        return false;
    }

    FVector Direction = ViewVector;
    if (Direction.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ApplyCameraViewVector: ViewVector is zero"));
        return false;
    }

    Direction.Normalize();

    const FVector CurrentLocation = PC->PlayerCameraManager->GetCameraLocation();
    const FRotator NewRotation = Direction.Rotation();

    PC->SetInitialLocationAndRotation(CurrentLocation, NewRotation);
    PC->SetControlRotation(NewRotation);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] ApplyCameraViewVector: ViewVector=%s CameraLocation=%s Rotation=%s Blend=%.2f"),
        *ViewVector.ToString(),
        *CurrentLocation.ToString(),
        *NewRotation.ToString(),
        BlendSeconds);

    return true;
}

bool ASystemSceneBuilder::DebugFindBodyByName(const FString& BodyName) const
{
    FSpawnedSystemBody FoundBody;
    const bool bFound = FindSpawnedBodyByName(BodyName, FoundBody);

    if (bFound)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] DebugFindBodyByName: FOUND '%s' -> Actor=%s Loc=%s Parent=%s"),
            *BodyName,
            *GetNameSafe(FoundBody.Actor),
            *FoundBody.SpawnLocation.ToString(),
            *GetNameSafe(FoundBody.ParentActor));
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] DebugFindBodyByName: FAILED '%s'"),
            *BodyName);
    }

    return bFound;
}

bool ASystemSceneBuilder::DebugFocusCameraOnBodyByName(
    const FString& BodyName,
    const FVector& CameraOffset)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] DebugFocusCameraOnBodyByName: World is null"));
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] DebugFocusCameraOnBodyByName: PlayerController is null"));
        return false;
    }

    FSpawnedSystemBody FoundBody;
    if (!FindSpawnedBodyByName(BodyName, FoundBody))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] DebugFocusCameraOnBodyByName: no body found for '%s'"),
            *BodyName);
        return false;
    }

    const FVector FocusPoint = FoundBody.SpawnLocation;
    FVector SceneOffset = ConvertLegacyCameraOffsetToSceneOffset(CameraOffset);

    const bool bIsPlanetOrMoon =
        !FoundBody.bIsOrbit &&
        (FoundBody.bIsMoon || FoundBody.ParentActor != nullptr);

    if (bIsPlanetOrMoon)
    {
        SceneOffset *= ScaleSettings.LegacyPlanetCameraFactor;

        const float VisualRadius = FMath::Max(FoundBody.VisualRadiusUnits, 1.0f);
        const float MinPlanetCameraDistance = FMath::Max(VisualRadius * 5.0f, 2000.0f);
        const float MaxPlanetCameraDistance = FMath::Max(VisualRadius * 12.0f, 20000.0f);

        const float Dist = SceneOffset.Size();
        if (Dist > KINDA_SMALL_NUMBER)
        {
            const FVector Dir = SceneOffset.GetSafeNormal();
            const float ClampedDist = FMath::Clamp(
                Dist,
                MinPlanetCameraDistance,
                MaxPlanetCameraDistance);

            SceneOffset = Dir * ClampedDist;
        }
        else
        {
            SceneOffset = FVector(-MinPlanetCameraDistance, -MinPlanetCameraDistance * 0.20f, MinPlanetCameraDistance * 0.60f);
        }
    }

    if (SceneOffset.IsNearlyZero())
    {
        SceneOffset = FVector(-2500.0f, -500.0f, 1500.0f);
    }

    const FVector CameraLocation = FocusPoint + SceneOffset;
    const FRotator CameraRotation = (FocusPoint - CameraLocation).Rotation();

    PC->SetInitialLocationAndRotation(CameraLocation, CameraRotation);
    PC->SetControlRotation(CameraRotation);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] DebugFocusCameraOnBodyByName: Target='%s' Focus=%s LegacyOffset=%s SceneOffset=%s VisualRadius=%.2f PlanetOrMoon=%s Camera=%s Rotation=%s"),
        *BodyName,
        *FocusPoint.ToString(),
        *CameraOffset.ToString(),
        *SceneOffset.ToString(),
        FoundBody.VisualRadiusUnits,
        bIsPlanetOrMoon ? TEXT("true") : TEXT("false"),
        *CameraLocation.ToString(),
        *CameraRotation.ToString());

    return true;
}

TSubclassOf<AActor> ASystemSceneBuilder::ResolvePlanetActorClass(EPlanetType PlanetType, bool bIsMoon) const
{
    TSubclassOf<AActor> ResultClass = nullptr;

    switch (PlanetType)
    {
    case EPlanetType::GasGiant:
        if (!bIsMoon && GasGiantPlanetActorClass)
        {
            ResultClass = GasGiantPlanetActorClass;
        }
        break;

    case EPlanetType::Barren:
        if (BarrenPlanetActorClass)
        {
            ResultClass = BarrenPlanetActorClass;
        }
        break;

    case EPlanetType::Ice:
        if (IcePlanetActorClass)
        {
            ResultClass = IcePlanetActorClass;
        }
        break;

    case EPlanetType::Terran:
        if (TerranPlanetActorClass)
        {
            ResultClass = TerranPlanetActorClass;
        }
        break;

    case EPlanetType::Volcanic:
        if (VolcanicPlanetActorClass)
        {
            ResultClass = VolcanicPlanetActorClass;
        }
        break;

    case EPlanetType::Unknown:
    default:
        break;
    }

    if (!ResultClass)
    {
        if (bIsMoon)
        {
            if (MoonActorClass)
            {
                ResultClass = MoonActorClass;
            }
            else if (BarrenPlanetActorClass)
            {
                ResultClass = BarrenPlanetActorClass;
            }
            else if (TerranPlanetActorClass)
            {
                ResultClass = TerranPlanetActorClass;
            }
        }
        else
        {
            if (TerranPlanetActorClass)
            {
                ResultClass = TerranPlanetActorClass;
            }
            else
            {
                ResultClass = PlanetActorClass;
            }
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetActorClass FALLBACK Type=%d bIsMoon=%s -> %s"),
            (int32)PlanetType,
            bIsMoon ? TEXT("true") : TEXT("false"),
            *GetNameSafe(ResultClass.Get()));
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetActorClass Type=%d bIsMoon=%s -> %s"),
            (int32)PlanetType,
            bIsMoon ? TEXT("true") : TEXT("false"),
            *GetNameSafe(ResultClass.Get()));
    }

    return ResultClass;
}

EPlanetType ASystemSceneBuilder::ResolvePlanetTypeFromData(
    const FString& BodyName,
    bool bIsMoon) const
{
    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        return EPlanetType::Unknown;
    }

    const FString SearchName = BodyName.TrimStartAndEnd();

    if (bIsMoon)
    {
        const FMoon* Moon = Env->FindMoonMapByName(SearchName);
        if (Moon)
        {
            return Moon->PlanetType;
        }

        return EPlanetType::Barren;
    }

    const FPlanet* Planet = Env->FindPlanetMapByName(SearchName);
    if (!Planet)
    {
        return EPlanetType::Unknown;
    }

    // Explicit data wins first.
    if (Planet->PlanetType == EPlanetType::GasGiant)
    {
        return EPlanetType::GasGiant;
    }

    // Starshatter legacy data often omits PlanetType for large gas worlds.
    const bool bLooksLikeGasGiantByTexture =
        Planet->Texture.Contains(TEXT("GasGiant"), ESearchCase::IgnoreCase);

    const bool bLooksLikeGasGiantByRadius =
        Planet->Radius >= 15000000.0f; // meters: Jalah=19,100,000, Trellis=30,000,000

    if (bLooksLikeGasGiantByTexture || bLooksLikeGasGiantByRadius)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: inferred GasGiant '%s' Radius=%.2f Texture='%s'"),
            *SearchName,
            Planet->Radius,
            *Planet->Texture);

        return EPlanetType::GasGiant;
    }

    return Planet->PlanetType;
}

void ASystemSceneBuilder::BuildRuntimeRegionForBody(
    const FString& BodyName,
    const FVector& BodyWorldLocation,
    AActor* BodyActor,
    float BodyVisualRadiusUnits,
    bool bIsMoon)
{
    if (!bSpawnRegionActors)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeRegionForBody: bSpawnRegionActors is false"));
        return;
    }

    if (BodyName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeRegionForBody: BodyName is empty"));
        return;
    }

    if (!BodyActor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] BuildRuntimeRegionForBody: BodyActor is null for '%s'"),
            *BodyName);
        return;
    }

    const float StandardRegionRadiusUnits =
        ConvertOrbitKmToSceneUnits(DefaultRegionRadiusKm, bIsMoon);

    const float GridUnits =
        ConvertOrbitKmToSceneUnits(DefaultRegionGridKm, bIsMoon);

    const float InnerRadiusUnits =
        FMath::Max(BodyVisualRadiusUnits, 1.0f);

    const float OuterRadiusUnits =
        InnerRadiusUnits + StandardRegionRadiusUnits;

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] BuildRuntimeRegionForBody: Body='%s' Loc=%s Inner=%.2f Outer=%.2f Grid=%.2f bIsMoon=%s BodyActor=%s"),
        *BodyName,
        *BodyWorldLocation.ToString(),
        InnerRadiusUnits,
        OuterRadiusUnits,
        GridUnits,
        bIsMoon ? TEXT("true") : TEXT("false"),
        *GetNameSafe(BodyActor));

    AActor* RegionActor = SpawnRegionActor(
        BodyName,
        BodyWorldLocation,
        OuterRadiusUnits,
        BodyActor);

    /*
     * IMPORTANT:
     * Do NOT call RegionActor->SetActorHiddenInGame(true) here.
     * The region actor is attached to the planet/moon actor.
     * Hide only its mesh inside SpawnRegionActor().
     */

    RegisterSpawnedRegion(
        BodyName + TEXT("_REGION"),
        BodyName,
        RegionActor,
        BodyActor,
        BodyWorldLocation,
        InnerRadiusUnits,
        OuterRadiusUnits,
        GridUnits);

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] REGION '%s_REGION' Anchor='%s' Loc=%s Inner=%.2f Outer=%.2f Grid=%.2f Actor=%s HiddenActor=%d"),
            *BodyName,
            *BodyName,
            *BodyWorldLocation.ToString(),
            InnerRadiusUnits,
            OuterRadiusUnits,
            GridUnits,
            *GetNameSafe(RegionActor),
            RegionActor ? (RegionActor->IsHidden() ? 1 : 0) : -1);
    }
}

AActor* ASystemSceneBuilder::SpawnRegionActor(
    const FString& RegionName,
    const FVector& WorldLocation,
    float OuterRadiusUnits,
    AActor* ParentActor)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnRegionActor: World is null for '%s'"),
            *RegionName);
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const FString FullName = RegionName + TEXT("_REGION");

    AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(),
        WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] SpawnRegionActor: failed to spawn '%s'"),
            *FullName);
        return nullptr;
    }

#if WITH_EDITOR
    Spawned->SetActorLabel(FullName);
#endif

    UStaticMeshComponent* SMC = Spawned->GetStaticMeshComponent();
    if (SMC)
    {
        SMC->SetMobility(EComponentMobility::Movable);

        static UStaticMesh* SphereMesh = nullptr;
        if (!SphereMesh)
        {
            SphereMesh = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        }

        if (SphereMesh)
        {
            SMC->SetStaticMesh(SphereMesh);
        }

        SMC->SetVisibility(true, true);
        SMC->SetHiddenInGame(true, true);
        SMC->SetCastShadow(false);
        SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        UE_LOG(LogTemp, Error,
            TEXT("[RegionDebug] %s ActorHidden=%d MeshVisible=%d MeshHiddenInGame=%d Mat=%s"),
            *FullName,
            Spawned->IsHidden() ? 1 : 0,
            SMC->IsVisible() ? 1 : 0,
            SMC->bHiddenInGame ? 1 : 0,
            *GetNameSafe(SMC->GetMaterial(0)));
    }

    Spawned->SetActorScale3D(FVector(OuterRadiusUnits));

    if (ParentActor)
    {
        Spawned->AttachToActor(
            ParentActor,
            FAttachmentTransformRules::KeepWorldTransform);
    }
    else
    {
        Spawned->AttachToActor(
            this,
            FAttachmentTransformRules::KeepWorldTransform);
    }

    SpawnedActors.Add(Spawned);

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] Spawned region '%s' Actor=%s Loc=%s OuterRadiusUnits=%.2f Parent=%s"),
        *FullName,
        *GetNameSafe(Spawned),
        *WorldLocation.ToString(),
        OuterRadiusUnits,
        *GetNameSafe(ParentActor));

    return Spawned;
}

void ASystemSceneBuilder::RegisterSpawnedBody(
    const FString& BodyName,
    AActor* Actor,
    AActor* ParentActor,
    bool bIsMoon,
    bool bIsOrbit,
    const FVector& SpawnLocation,
    float VisualRadiusUnits,
    float OrbitRadiusUnits)
{
    FSpawnedSystemBody Entry;
    Entry.BodyName = BodyName;
    Entry.Actor = Actor;
    Entry.ParentActor = ParentActor;
    Entry.bIsMoon = bIsMoon;
    Entry.bIsOrbit = bIsOrbit;
    Entry.SpawnLocation = SpawnLocation;
    Entry.VisualRadiusUnits = VisualRadiusUnits;
    Entry.OrbitRadiusUnits = OrbitRadiusUnits;

    UE_LOG(LogTemp, Error,
        TEXT("[REGISTER BODY] Name=%s Actor=%s ActorLoc=%s SpawnLoc=%s RadiusUnits=%.2f OrbitUnits=%.2f Scale=%s IsMoon=%d IsOrbit=%d Parent=%s"),
        *BodyName,
        *GetNameSafe(Actor),
        Actor ? *Actor->GetActorLocation().ToString() : TEXT("NULL"),
        *SpawnLocation.ToString(),
        VisualRadiusUnits,
        OrbitRadiusUnits,
        Actor ? *Actor->GetActorScale3D().ToString() : TEXT("NULL"),
        bIsMoon ? 1 : 0,
        bIsOrbit ? 1 : 0,
        *GetNameSafe(ParentActor));

    SpawnedBodies.Add(Entry);
}

void ASystemSceneBuilder::RegisterSpawnedRegion(
    const FString& RegionName,
    const FString& AnchorBodyName,
    AActor* Actor,
    AActor* ParentActor,
    const FVector& SpawnLocation,
    float InnerRadiusUnits,
    float OuterRadiusUnits,
    float GridUnits)
{
    FSpawnedSystemRegion Entry;
    Entry.RegionName = RegionName;
    Entry.AnchorBodyName = AnchorBodyName;
    Entry.Actor = Actor;
    Entry.ParentActor = ParentActor;
    Entry.SpawnLocation = SpawnLocation;
    Entry.InnerRadiusUnits = InnerRadiusUnits;
    Entry.OuterRadiusUnits = OuterRadiusUnits;
    Entry.GridUnits = GridUnits;

    SpawnedRegions.Add(Entry);

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] RegisterSpawnedRegion: Region='%s' Anchor='%s' Inner=%.2f Outer=%.2f Count=%d"),
        *RegionName,
        *AnchorBodyName,
        InnerRadiusUnits,
        OuterRadiusUnits,
        SpawnedRegions.Num());
}

bool ASystemSceneBuilder::GetRegionWorldLocationByName(
    const FString& RegionName,
    FVector& OutWorldLocation) const
{
    OutWorldLocation = FVector::ZeroVector;

    FSpawnedSystemRegion FoundRegion;
    if (!GetRegionByName(RegionName, FoundRegion))
    {
        return false;
    }

    if (FoundRegion.Actor)
    {
        OutWorldLocation = FoundRegion.Actor->GetActorLocation();
        return true;
    }

    OutWorldLocation = FoundRegion.SpawnLocation;
    return true;
}

bool ASystemSceneBuilder::GetRegionByName(
    const FString& RegionName,
    FSpawnedSystemRegion& OutRegion) const
{
    const FString SearchName = RegionName.TrimStartAndEnd();

    if (SearchName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] GetRegionByName: empty RegionName"));
        return false;
    }

    for (const FSpawnedSystemRegion& Entry : SpawnedRegions)
    {
        const FString EntryName = Entry.RegionName.TrimStartAndEnd();

        if (EntryName.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            OutRegion = Entry;

            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] GetRegionByName: '%s' -> Anchor='%s' Inner=%.2f Outer=%.2f"),
                *SearchName,
                *OutRegion.AnchorBodyName,
                OutRegion.InnerRadiusUnits,
                OutRegion.OuterRadiusUnits);

            return true;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] GetRegionByName: no match for '%s'"),
        *SearchName);

    return false;
}

void ASystemSceneBuilder::SpawnDirectionalLight()
{
    if (!GetWorld())
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ADirectionalLight* SunLight =
        GetWorld()->SpawnActor<ADirectionalLight>(
            ADirectionalLight::StaticClass(),
            FVector::ZeroVector,
            FRotator(-35.0f, 45.0f, 0.0f),
            Params);

    if (!SunLight)
    {
        return;
    }

    UDirectionalLightComponent* LightComp =
        SunLight->FindComponentByClass<UDirectionalLightComponent>();

    if (!LightComp)
    {
        return;
    }

    LightComp->SetMobility(EComponentMobility::Movable);
    LightComp->SetIntensity(10.0f);
    LightComp->SetLightColor(FLinearColor(1.0f, 0.95f, 0.85f));
    LightComp->SetCastShadows(false);
}

void ASystemSceneBuilder::SpawnSkyLight()
{
    // Prevent duplicate skylights
    if (SceneSkyLight)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] SpawnSkyLight: already exists"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] SpawnSkyLight: World is null"));
        return;
    }

    // Spawn actor
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SceneSkyLight = World->SpawnActor<ASkyLight>(
        ASkyLight::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params);

    if (!SceneSkyLight)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] SpawnSkyLight: spawn failed"));
        return;
    }

    // Get component
    USkyLightComponent* SkyComp = SceneSkyLight->GetLightComponent();
    if (!SkyComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] SpawnSkyLight: SkyLightComponent missing"));
        return;
    }

    // Mobility (required for runtime scenes)
    SkyComp->SetMobility(EComponentMobility::Movable);

    // Use captured scene (no cubemap required)
    SkyComp->SourceType = ESkyLightSourceType::SLS_CapturedScene;

    // Global ambient strength
    SkyComp->Intensity = 1.0f;

    SkyComp->SetLightColor(FLinearColor(0.6f, 0.7f, 1.0f));

    // Important for space scenes (prevents weird ground lighting)
    SkyComp->bLowerHemisphereIsBlack = true;

    // Ensure it updates immediately
    SkyComp->RecaptureSky();

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] SpawnSkyLight: created intensity=%.2f"),
        SkyComp->Intensity);
}

void ASystemSceneBuilder::SetTemporaryCutsceneBodyScale(
    const FString& BodyName,
    float ScaleMultiplier)
{
    if (BodyName.IsEmpty() || ScaleMultiplier <= 0.0f)
    {
        return;
    }

    const FString SearchName = BodyName.TrimStartAndEnd();

    for (FSpawnedSystemBody& Body : SpawnedBodies)
    {
        if (Body.bIsOrbit || !Body.Actor)
        {
            continue;
        }

        if (!Body.BodyName.Equals(SearchName, ESearchCase::IgnoreCase))
        {
            continue;
        }

        AActor* BodyActor = Body.Actor;
        if (!BodyActor)
        {
            return;
        }

        if (!OriginalCutsceneBodyScales.Contains(BodyActor))
        {
            OriginalCutsceneBodyScales.Add(
                BodyActor,
                BodyActor->GetActorScale3D());
        }

        const FVector OriginalScale = OriginalCutsceneBodyScales[BodyActor];
        const FVector NewScale = OriginalScale * ScaleMultiplier;

        BodyActor->SetActorScale3D(NewScale);

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Cutscene body scale: Body='%s' Multiplier=%.2f Original=%s New=%s"),
            *Body.BodyName,
            ScaleMultiplier,
            *OriginalScale.ToString(),
            *NewScale.ToString());

        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] Cutscene body scale failed: Body='%s' not found"),
        *BodyName);
}

void ASystemSceneBuilder::ResetTemporaryCutsceneBodyScales()
{
    for (TPair<TObjectPtr<AActor>, FVector>& Pair : OriginalCutsceneBodyScales)
    {
        if (Pair.Key)
        {
            Pair.Key->SetActorScale3D(Pair.Value);
        }
    }

    OriginalCutsceneBodyScales.Empty();
}