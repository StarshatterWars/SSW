/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
        John DiCamillo / Destroyer Studios LLC

    AUTHOR:
        Carlos Bott

    FILE:
        CampaignSceneActor.cpp

    MODULE:
        Campaign Scene System

    OVERVIEW:
        Runtime implementation of cutscene scene actor system.

        Responsible for:
        - Spawning ships/stations from mission data
        - Maintaining name-based lookup
        - Camera targeting for cutscene events

=============================================================================*/

#include "CampaignSceneActor.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"

ACampaignSceneActor::ACampaignSceneActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void ACampaignSceneActor::BeginPlay()
{
    Super::BeginPlay();
}

void ACampaignSceneActor::ClearSceneActors()
{
    for (AActor* Actor : OwnedActors)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    OwnedActors.Empty();
    SpawnedSceneActors.Empty();

    if (bEnableDebugLogs)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Cleared scene actors"));
    }
}

FVector ACampaignSceneActor::ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const
{
    return GetActorLocation() + SceneOriginOffset + (LegacyLoc * LegacyUnitsPerKm);
}

void ACampaignSceneActor::BuildSceneActorsFromMission(const FS_CampaignMission& MissionData)
{
    ClearSceneActors();

    for (const FS_MissionElement& Elem : MissionData.Element)
    {
        if (Elem.Name.IsEmpty())
        {
            continue;
        }

        const FVector WorldLoc = ConvertLegacySceneLocToWorld(Elem.Location);

        AActor* Spawned = SpawnSceneElementActor(
            Elem.Name,
            Elem.Design,
            WorldLoc);

        if (!Spawned)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CampaignSceneActor] Failed spawn Name='%s' Design='%s'"),
                *Elem.Name,
                *Elem.Design);
            continue;
        }

        FCampaignSceneSpawnedActor Entry;
        Entry.ElementName = Elem.Name;
        Entry.DesignName = Elem.Design;
        Entry.Actor = Spawned;
        Entry.SpawnLocation = WorldLoc;

        SpawnedSceneActors.Add(Entry);
        OwnedActors.Add(Spawned);

        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Spawned '%s' (%s)"),
            *Elem.Name,
            *Elem.Design);
    }
}

AActor* ACampaignSceneActor::SpawnSceneElementActor(
    const FString& ElementName,
    const FString& DesignName,
    const FVector& WorldLocation)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    TSubclassOf<AActor> SpawnClass = ResolveActorClassForDesign(DesignName);

    if (!SpawnClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] No class for design '%s'"),
            *DesignName);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Actor = World->SpawnActor<AActor>(
        SpawnClass,
        WorldLocation,
        FRotator::ZeroRotator,
        Params);

    if (Actor)
    {
#if WITH_EDITOR
        Actor->SetActorLabel(ElementName);
#endif
        Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    }

    return Actor;
}

TSubclassOf<AActor> ACampaignSceneActor::ResolveActorClassForDesign(const FString& DesignName) const
{
    if (const TSubclassOf<AActor>* Found = DesignActorClasses.Find(DesignName))
    {
        return *Found;
    }

    return nullptr;
}

bool ACampaignSceneActor::FindSceneActorByName(
    const FString& TargetName,
    FCampaignSceneSpawnedActor& OutEntry) const
{
    for (const FCampaignSceneSpawnedActor& Entry : SpawnedSceneActors)
    {
        if (Entry.ElementName.Equals(TargetName, ESearchCase::IgnoreCase))
        {
            OutEntry = Entry;
            return true;
        }
    }

    return false;
}

bool ACampaignSceneActor::FocusCameraOnSceneActorByName(
    const FString& TargetName,
    const FVector& CameraOffset,
    float BlendSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }

    FCampaignSceneSpawnedActor Entry;
    if (!FindSceneActorByName(TargetName, Entry) || !Entry.Actor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CampaignSceneActor] Target not found '%s'"),
            *TargetName);
        return false;
    }

    const FVector Focus = Entry.Actor->GetActorLocation();

    FVector Offset = CameraOffset;
    if (Offset.IsNearlyZero())
    {
        Offset = FVector(-2500.0f, -750.0f, 1200.0f);
    }

    const FVector CamLoc = Focus + Offset;
    const FRotator CamRot = (Focus - CamLoc).Rotation();

    PC->SetInitialLocationAndRotation(CamLoc, CamRot);
    PC->SetControlRotation(CamRot);

    if (PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager->SetGameCameraCutThisFrame();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CampaignSceneActor] Camera focus '%s'"),
        *TargetName);

    return true;
}