#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "CampaignSceneActor.generated.h"

class ASceneMeshActor;
class ASystemSceneBuilder;
class ASSWCameraManager;
class Mission;

class Ship;
class AShipActor;
struct FS_MissionElement;

USTRUCT()
struct FCampaignSceneSpawnedActor
{
    GENERATED_BODY()

    UPROPERTY() FString ElementName;
    UPROPERTY() FString DesignName;
    UPROPERTY() FString RegionName;
    UPROPERTY() FString CommanderName; // ?? ADD THIS
    UPROPERTY() AActor* Actor = nullptr;
    UPROPERTY() FVector SpawnLocation = FVector::ZeroVector;
    UPROPERTY() int32 HeadingDegrees = 0;
};

UCLASS()
class STARSHATTERWARS_API ACampaignSceneActor : public AActor
{
    GENERATED_BODY()

public:
    ACampaignSceneActor();

    virtual void BeginPlay() override;

    void BuildSceneActorsFromRuntimeMission(Mission* MissionPtr);
    void BuildSceneActorsFromMission(const FS_CampaignMission& MissionData);

    void ClearSceneActors();
    void DumpSceneActors() const;

    ASSWCameraManager* ResolveSSWCameraManager() const;

public:
    FVector ConvertLegacyRegionOffsetToSceneOffset(const FVector& LegacyOffset) const;
    FVector ConvertLegacyRegionOffsetToWorld(AActor* RegionActor, const FVector& LegacyOffset) const;

    float ConvertLegacyRegionSpeedToSceneSpeed(float LegacySpeed) const;

    FString GetCommanderForElement(const FString& ElementName) const;

    bool FocusCameraOnSceneActorGroup(const TArray<FString>& ElementNames, float BlendSeconds) const;

    int32 GetCommanderGroupActorCount(const FString& CommanderName) const;

    bool FocusCameraOnCommanderGroup(const FString& CommanderName, float BlendSeconds) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Scale")
    float MissionRegionOffsetScale = 8300.0f;

public:
    bool FocusCameraOnSceneActorByName(
        const FString& ElementName,
        const FVector& CameraOffset,
        float BlendSeconds = 0.0f) const;

    bool FocusCameraOnSceneActorByName(
        const FString& ElementName,
        const FVector& CameraOffset,
        const FRotator& CameraRotator,
        float BlendSeconds) const;

    AActor* FindSceneActorByName(const FString& ElementName) const;

protected:
    ASystemSceneBuilder* ResolveSystemSceneBuilder() const;

    FVector ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const;
    FVector ConvertMissionElementLocToWorld(const FS_MissionElement& Elem) const;

    bool ResolveRegionAnchorLocation(const FString& RegionName, FVector& OutWorldLocation) const;
    bool ResolveRegionCenterLocation(const FString& RegionName, FVector& OutWorldLocation) const;

    FString ResolveModelNameForDesign(const FString& DesignName) const;
    FString ResolveMeshPathForDesign(const FString& DesignName) const;
    UStaticMesh* ResolveStaticMeshFromPath(const FString& MeshPath) const;

    AActor* FindRegionActorByName(const FString& RegionName) const;

    AActor* SpawnSceneElementActor(
        const FString& ElementName,
        const FString& DesignName,
        const FVector& WorldLocation,
        int32 HeadingDegrees);

protected:
    UPROPERTY()
    USceneComponent* SceneRoot;

    UPROPERTY(EditAnywhere, Category = "Scene")
    TSubclassOf<ASceneMeshActor> DefaultSceneMeshActorClass;

    UPROPERTY(EditAnywhere, Category = "Scene")
    FVector SceneOriginOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Scene")
    float LegacyUnitsPerKm = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bEnableDebugLogs = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Scaling")
    float ShipLengthToSceneUnits = 0.01f;

    UPROPERTY()
    TArray<AActor*> OwnedActors;

    UPROPERTY()
    TArray<FCampaignSceneSpawnedActor> SpawnedSceneActors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|MissionElements")
    float MissionElementScaleMultiplier = 1.0f;

private:
    Ship* CreateRuntimeShipForMissionElement(const FS_MissionElement& Elem);
    void BindRuntimeShipToActor(AShipActor* ShipActor, const FS_MissionElement& Elem);
};