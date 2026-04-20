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

#include "StarSystem.h"
#include "Orbital.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "GasGiantActor.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
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
}

void ASystemSceneBuilder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ++TickCounter;
    DebugTickAccumulator += DeltaTime;

    RefreshRuntimeBodyTransforms();

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

void ASystemSceneBuilder::ClearSpawnedBodies()
{
    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ClearSpawnedBodies Actors=%d Bodies=%d RuntimeBodyMap=%d BodyActorMap=%d"),
            SpawnedActors.Num(),
            SpawnedBodies.Num(),
            RuntimeBodyMap.Num(),
            BodyActorMap.Num());
    }

    for (AActor* Spawned : SpawnedActors)
    {
        if (IsValid(Spawned))
        {
            Spawned->Destroy();
        }
    }

    SpawnedActors.Empty();
    SpawnedBodies.Empty();
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
    const FVector RuntimeOffsetKm = PlanetBody->Location() - PrimaryStarRuntimeLocation;
    const FVector SceneOffset = ConvertRuntimeOffsetToSceneOffset(RuntimeOffsetKm, false);
    const FVector PlanetWorldLocation = StarAnchorWorldLocation + SceneOffset;

    const EPlanetType PlanetType = ResolvePlanetTypeFromData(PlanetName, false);

    const float PlanetRadiusKm =
        PlanetBody->Radius() > 0.0 ? (float)PlanetBody->Radius() : 2000.0f;

    const float RadiusUnits =
        (PlanetType == EPlanetType::GasGiant)
        ? ConvertGasGiantRadiusKmToSceneUnits(PlanetRadiusKm)
        : ConvertPlanetRadiusKmToSceneUnits(PlanetRadiusKm);

    const float OrbitRadiusUnits = ConvertOrbitKmToSceneUnits(
        (float)RuntimeOffsetKm.Size(),
        false);

    if (bSpawnOrbitActors && OrbitActorClass && OrbitRadiusUnits > 0.0f)
    {
        SpawnOrbitActor(
            PlanetName,
            StarAnchorWorldLocation,
            OrbitRadiusUnits,
            ParentActor);
    }

    const TSubclassOf<AActor> ResolvedPlanetClass =
        ResolvePlanetActorClass(PlanetType, false);

    AActor* PlanetActor = SpawnBodyActor(
        ResolvedPlanetClass,
        PlanetName,
        PlanetWorldLocation,
        RadiusUnits,
        ParentActor,
        false,
        false);

    RegisterSpawnedBody(
        PlanetName,
        PlanetActor,
        ParentActor,
        false,
        false,
        PlanetWorldLocation,
        RadiusUnits,
        OrbitRadiusUnits);

    TrackRuntimeBody(PlanetName, PlanetBody, PlanetActor);

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] PLANET '%s' Type=%d Class=%s RuntimeLoc=%s RuntimeOffset=%s SceneOffset=%s SceneLoc=%s OrbitKm=%.2f OrbitUnits=%.2f RadiusKm=%.2f RadiusUnits=%.2f Parent=%s"),
            *PlanetName,
            (int32)PlanetType,
            *GetNameSafe(ResolvedPlanetClass.Get()),
            *PlanetBody->Location().ToString(),
            *RuntimeOffsetKm.ToString(),
            *SceneOffset.ToString(),
            *PlanetWorldLocation.ToString(),
            RuntimeOffsetKm.Size(),
            OrbitRadiusUnits,
            PlanetRadiusKm,
            RadiusUnits,
            *GetNameSafe(ParentActor));
    }

    ListIter<OrbitalBody> MoonIter = PlanetBody->Satellites();
    while (++MoonIter)
    {
        OrbitalBody* MoonBody = MoonIter.value();
        if (!MoonBody)
        {
            continue;
        }

        BuildRuntimeMoon(
            MoonBody,
            StarAnchorWorldLocation,
            PrimaryStarRuntimeLocation,
            PlanetActor);
    }
}

void ASystemSceneBuilder::BuildRuntimeMoon(
    OrbitalBody* MoonBody,
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

    const FString MoonName = ANSI_TO_TCHAR(MoonBody->GetName());
    const FVector RuntimeOffsetKm = MoonBody->Location() - PrimaryStarRuntimeLocation;
    const FVector SceneOffset = ConvertRuntimeOffsetToSceneOffset(RuntimeOffsetKm, true);
    const FVector MoonWorldLocation = StarAnchorWorldLocation + SceneOffset;

    const float RadiusUnits = ConvertMoonRadiusKmToSceneUnits(
        MoonBody->Radius() > 0.0 ? (float)MoonBody->Radius() : 500.0f);

    const float OrbitRadiusUnits = ConvertOrbitKmToSceneUnits(
        (float)RuntimeOffsetKm.Size(),
        true);

    const EPlanetType PlanetType = ResolvePlanetTypeFromData(MoonName, true);

    if (bSpawnOrbitActors && OrbitActorClass && OrbitRadiusUnits > 0.0f)
    {
        SpawnOrbitActor(
            MoonName,
            StarAnchorWorldLocation,
            OrbitRadiusUnits,
            ParentActor);
    }

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

    TrackRuntimeBody(MoonName, MoonBody, MoonActor);

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] MOON '%s' Type=%d Class=%s RuntimeLoc=%s RuntimeOffset=%s SceneOffset=%s SceneLoc=%s OrbitKm=%.2f OrbitUnits=%.2f RadiusKm=%.2f RadiusUnits=%.2f Parent=%s"),
            *MoonName,
            (int32)PlanetType,
            *GetNameSafe(ResolvedMoonClass.Get()),
            *MoonBody->Location().ToString(),
            *RuntimeOffsetKm.ToString(),
            *SceneOffset.ToString(),
            *MoonWorldLocation.ToString(),
            RuntimeOffsetKm.Size(),
            OrbitRadiusUnits,
            MoonBody->Radius(),
            RadiusUnits,
            *GetNameSafe(ParentActor));
    }
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

        Actor->SetActorLocation(NewLocation);

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

    FString TypeSuffix = TEXT("PLANET");
    if (bIsStar)
    {
        TypeSuffix = TEXT("STAR");
    }
    else if (bIsMoon)
    {
        TypeSuffix = TEXT("MOON");
    }

    const FString FullName = BodyName + TEXT("_") + TypeSuffix;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // Do NOT set SpawnParams.Name for now

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] SpawnBodyActor: Class=%s FullName=%s World=%s Loc=%s"),
        *GetNameSafe(BodyClass.Get()),
        *FullName,
        *GetNameSafe(World),
        *WorldLocation.ToString());

    AActor* Spawned = World->SpawnActor<AActor>(
        BodyClass,
        WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    if (!Spawned)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[SystemSceneBuilder] FAILED to spawn '%s' Class=%s"),
            *FullName,
            *GetNameSafe(BodyClass.Get()));
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
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] Failed to load debug sphere mesh for '%s'"),
                *BodyName);
        }
    }

    if (AGasGiantActor* GasGiant = Cast<AGasGiantActor>(Spawned))
    {
        Spawned->SetActorScale3D(FVector(1.0f));

        const float GasGiantRadiusForBP = FMath::Max(1.0f, VisualRadiusUnits);
        GasGiant->SetPlanetRadius(GasGiantRadiusForBP);

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] SpawnBodyActor: Gas giant '%s' VisualRadiusUnits=%.2f BPRadius=%.2f"),
            *BodyName,
            VisualRadiusUnits,
            GasGiantRadiusForBP);
    }
    else
    {
        Spawned->SetActorScale3D(FVector(VisualRadiusUnits));
    }

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

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] Spawned '%s' Actor=%s Loc=%s Scale=%s Parent=%s bIsStar=%s bIsMoon=%s"),
            *FullName,
            *GetNameSafe(Spawned),
            *WorldLocation.ToString(),
            *Spawned->GetActorScale3D().ToString(),
            *GetNameSafe(ParentActor),
            bIsStar ? TEXT("true") : TEXT("false"),
            bIsMoon ? TEXT("true") : TEXT("false"));
    }

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

    SpawnedBodies.Add(Entry);
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

float ASystemSceneBuilder::ConvertPlanetRadiusKmToSceneUnits(float RadiusKm) const
{
    const float Raw = USystemUtils::RadiusKmToSceneUnitsClamped(
        RadiusKm,
        ScaleSettings.RadiusUnitsPerThousandKm,
        ScaleSettings.MinBodyScaleUnits,
        ScaleSettings.MaxBodyScaleUnits);

    return FMath::Clamp(
        Raw,
        ScaleSettings.MinPlanetScaleUnits,
        ScaleSettings.MaxPlanetScaleUnits);
}

float ASystemSceneBuilder::ConvertGasGiantRadiusKmToSceneUnits(float RadiusKm) const
{
    const float Raw = USystemUtils::RadiusKmToSceneUnitsClamped(
        RadiusKm,
        ScaleSettings.RadiusUnitsPerThousandKm,
        ScaleSettings.MinBodyScaleUnits,
        ScaleSettings.MaxBodyScaleUnits);

    return FMath::Clamp(
        Raw * ScaleSettings.GasGiantScaleMultiplier,
        ScaleSettings.MinGasGiantScaleUnits,
        ScaleSettings.MaxGasGiantScaleUnits);
}

float ASystemSceneBuilder::ConvertMoonRadiusKmToSceneUnits(float RadiusKm) const
{
    const float Raw = USystemUtils::RadiusKmToSceneUnitsClamped(
        RadiusKm,
        ScaleSettings.RadiusUnitsPerThousandKm,
        ScaleSettings.MinBodyScaleUnits,
        ScaleSettings.MaxBodyScaleUnits);

    return FMath::Clamp(
        Raw,
        ScaleSettings.MinMoonScaleUnits,
        ScaleSettings.MaxMoonScaleUnits);
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

    auto IsStarEntry = [](const FSpawnedSystemBody& Entry) -> bool
        {
            return !Entry.bIsOrbit &&
                !Entry.bIsMoon &&
                Entry.ParentActor == nullptr;
        };

    const FSpawnedSystemBody* ExactNonStarMatch = nullptr;
    const FSpawnedSystemBody* PartialNonStarMatch = nullptr;
    const FSpawnedSystemBody* ExactStarMatch = nullptr;
    const FSpawnedSystemBody* PartialStarMatch = nullptr;

    for (const FSpawnedSystemBody& Entry : SpawnedBodies)
    {
        if (Entry.bIsOrbit)
        {
            continue;
        }

        const bool bIsStar = IsStarEntry(Entry);
        const bool bExact = Entry.BodyName.Equals(SearchName, ESearchCase::IgnoreCase);
        const bool bPartial = Entry.BodyName.Contains(SearchName, ESearchCase::IgnoreCase);

        if (bExact)
        {
            if (!bIsStar && !ExactNonStarMatch)
            {
                ExactNonStarMatch = &Entry;
            }
            else if (bIsStar && !ExactStarMatch)
            {
                ExactStarMatch = &Entry;
            }
        }
        else if (bPartial)
        {
            if (!bIsStar && !PartialNonStarMatch)
            {
                PartialNonStarMatch = &Entry;
            }
            else if (bIsStar && !PartialStarMatch)
            {
                PartialStarMatch = &Entry;
            }
        }
    }

    const FSpawnedSystemBody* BestMatch = nullptr;

    if (ExactNonStarMatch)
    {
        BestMatch = ExactNonStarMatch;
    }
    else if (PartialNonStarMatch)
    {
        BestMatch = PartialNonStarMatch;
    }
    else if (ExactStarMatch)
    {
        BestMatch = ExactStarMatch;
    }
    else if (PartialStarMatch)
    {
        BestMatch = PartialStarMatch;
    }

    if (!BestMatch)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FindSpawnedBodyByName: no match for '%s'"),
            *SearchName);
        return false;
    }

    OutBody = *BestMatch;

    UE_LOG(LogTemp, Warning,
        TEXT("[SystemSceneBuilder] FindSpawnedBodyByName: '%s' -> '%s' IsStar=%s IsMoon=%s"),
        *SearchName,
        *BestMatch->BodyName,
        IsStarEntry(*BestMatch) ? TEXT("true") : TEXT("false"),
        BestMatch->bIsMoon ? TEXT("true") : TEXT("false"));

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

    FSpawnedSystemBody FoundBody;
    if (!FindSpawnedBodyByName(BodyName, FoundBody))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: body not found '%s'"),
            *BodyName);
        return false;
    }

    FVector SceneOffset = ConvertLegacyCameraOffsetToSceneOffset(CameraOffset);
    
    FVector FocusPoint = FoundBody.SpawnLocation;

    if (!FoundBody.bIsOrbit)
    {
        const float Radius = FMath::Max(FoundBody.VisualRadiusUnits, 1.0f);

        // push focus point toward camera slightly (surface bias)
        const FVector ViewDir = SceneOffset.GetSafeNormal();
        const float SurfaceBias = Radius * 0.85f; // tweak: 0.5 - 0.8 range

        FocusPoint += ViewDir * SurfaceBias;
    }

    const bool bIsPlanetOrMoon =
        !FoundBody.bIsOrbit &&
        (FoundBody.bIsMoon || FoundBody.ParentActor != nullptr);

    if (bIsPlanetOrMoon)
    {
        SceneOffset *= ScaleSettings.LegacyPlanetCameraFactor;

        const float VisualRadius = FMath::Max(FoundBody.VisualRadiusUnits, 1.0f);
        const float MinPlanetCameraDistance = FMath::Max(VisualRadius * 4.5f, 1200.0f);
        const float MaxPlanetCameraDistance = FMath::Max(VisualRadius * 14.0f, 8000.0f);

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
        TEXT("[SystemSceneBuilder] FocusCameraOnBodyByName: Target='%s' Focus=%s LegacyOffset=%s SceneOffset=%s VisualRadius=%.2f PlanetOrMoon=%s Camera=%s Rotation=%s Blend=%.2f"),
        *BodyName,
        *FocusPoint.ToString(),
        *CameraOffset.ToString(),
        *SceneOffset.ToString(),
        FoundBody.VisualRadiusUnits,
        bIsPlanetOrMoon ? TEXT("true") : TEXT("false"),
        *CameraLocation.ToString(),
        *CameraRotation.ToString(),
        BlendSeconds);

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
        const float MinPlanetCameraDistance = FMath::Max(VisualRadius * 4.5f, 1200.0f);
        const float MaxPlanetCameraDistance = FMath::Max(VisualRadius * 14.0f, 8000.0f);

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

EPlanetType ASystemSceneBuilder::ResolvePlanetTypeFromData(const FString& BodyName, bool bIsMoon) const
{
    UStarshatterEnvironmentSubsystem* Env = GetEnvironmentSubsystem();
    if (!Env)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: Env is null '%s'"),
            *BodyName);
        return EPlanetType::Unknown;
    }

    const FString SearchName = BodyName.TrimStartAndEnd();

    if (bIsMoon)
    {
        const FMoon* Moon = Env->FindMoonMapByName(SearchName);
        if (Moon)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: Moon '%s' -> Type=%d"),
                *SearchName,
                (int32)Moon->PlanetType);
            return Moon->PlanetType;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: Moon lookup FAILED '%s'"),
            *SearchName);
    }
    else
    {
        const FPlanet* Planet = Env->FindPlanetMapByName(SearchName);
        if (Planet)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: Planet '%s' -> Type=%d"),
                *SearchName,
                (int32)Planet->PlanetType);
            return Planet->PlanetType;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[SystemSceneBuilder] ResolvePlanetTypeFromData: Planet lookup FAILED '%s'"),
            *SearchName);
    }

    return EPlanetType::Unknown;
}