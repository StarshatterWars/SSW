#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "CampaignSceneActor.generated.h"

class USceneComponent;
class UStaticMesh;
class ASceneMeshActor;
class ASystemSceneBuilder;

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

    void DumpSceneActors() const;

protected:
    FVector ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const;

    AActor* SpawnSceneElementActor(
        const FString& ElementName,
        const FString& DesignName,
        const FVector& WorldLocation);

    FString ResolveModelNameForDesign(const FString& DesignName) const;
    FString ResolveMeshPathForDesign(const FString& DesignName) const;
    UStaticMesh* ResolveStaticMeshFromPath(const FString& MeshPath) const;

    FVector ConvertMissionElementLocToWorld(const FS_MissionElement& Elem) const;
    ASystemSceneBuilder* ResolveSystemSceneBuilder() const;
    bool ResolveRegionAnchorLocation(
        const FString& RegionName,
        FVector& OutWorldLocation) const;

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(EditAnywhere, Category = "Campaign Scene")
    bool bEnableDebugLogs = true;

    UPROPERTY(EditAnywhere, Category = "Campaign Scene")
    float LegacyUnitsPerKm = 0.01f;

    UPROPERTY(EditAnywhere, Category = "Campaign Scene")
    FVector SceneOriginOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Campaign Scene")
    TSubclassOf<ASceneMeshActor> DefaultSceneMeshActorClass;

    UPROPERTY()
    TArray<FCampaignSceneSpawnedActor> SpawnedSceneActors;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> OwnedActors;
};