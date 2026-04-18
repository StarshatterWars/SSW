/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
        John DiCamillo / Destroyer Studios LLC

    AUTHOR:
        Carlos Bott

    FILE:
        CampaignSceneActor.h

    MODULE:
        Campaign Scene System

    OVERVIEW:
        Handles cutscene scene elements (ships, stations, etc.)
        separate from orbital system builder.

        - Spawns mission "element" actors
        - Maintains lookup by name
        - Supports camera focus on scene elements

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "CampaignSceneActor.generated.h"

USTRUCT()
struct FCampaignSceneSpawnedActor
{
    GENERATED_BODY()

    UPROPERTY()
    FString ElementName;

    UPROPERTY()
    FString DesignName;

    UPROPERTY()
    TObjectPtr<AActor> Actor = nullptr;

    UPROPERTY()
    FVector SpawnLocation = FVector::ZeroVector;
};

UCLASS()
class STARSHATTERWARS_API ACampaignSceneActor : public AActor
{
    GENERATED_BODY()

public:

    ACampaignSceneActor();

    virtual void BeginPlay() override;

    void ClearSceneActors();

    void BuildSceneActorsFromMission(const FS_CampaignMission& MissionData);

    bool FocusCameraOnSceneActorByName(
        const FString& TargetName,
        const FVector& CameraOffset,
        float BlendSeconds = 0.0f);

    bool FindSceneActorByName(
        const FString& TargetName,
        FCampaignSceneSpawnedActor& OutEntry) const;

protected:

    FVector ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const;

    AActor* SpawnSceneElementActor(
        const FString& ElementName,
        const FString& DesignName,
        const FVector& WorldLocation);

    TSubclassOf<AActor> ResolveActorClassForDesign(const FString& DesignName) const;

protected:

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere)
    bool bEnableDebugLogs = true;

    UPROPERTY(EditAnywhere)
    float LegacyUnitsPerKm = 0.01f;

    UPROPERTY(EditAnywhere)
    FVector SceneOriginOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere)
    TMap<FString, TSubclassOf<AActor>> DesignActorClasses;

    UPROPERTY()
    TArray<FCampaignSceneSpawnedActor> SpawnedSceneActors;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> OwnedActors;
};
