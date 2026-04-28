#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "CampaignSceneActor.generated.h"

class ASceneMeshActor;
class AShipActor;
class ASystemSceneBuilder;
class ASSWCameraManager;
class Mission;
class Ship;

struct FS_MissionElement;

USTRUCT()
struct FCampaignSceneSpawnedActor
{
    GENERATED_BODY()

    UPROPERTY()
    FString ElementName;

    UPROPERTY()
    FString DesignName;

    UPROPERTY()
    FString RegionName;

    UPROPERTY()
    FString CommanderName;

    UPROPERTY()
    AActor* Actor = nullptr;

    UPROPERTY()
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY()
    int32 HeadingDegrees = 0;
};

UCLASS()
class STARSHATTERWARS_API ACampaignSceneActor : public AActor
{
    GENERATED_BODY()

public:
    ACampaignSceneActor();

    virtual void BeginPlay() override;

    //-------------------------------------------------------------
    // Scene Build / Clear
    //-------------------------------------------------------------
    void BuildSceneActorsFromRuntimeMission(Mission* MissionPtr);
    void BuildSceneActorsFromMission(const FS_CampaignMission& MissionData);

    void ClearSceneActors();
    void DumpSceneActors() const;

    //-------------------------------------------------------------
    // Camera
    //-------------------------------------------------------------
    ASSWCameraManager* ResolveSSWCameraManager() const;

    bool FocusCameraOnSceneActorByName(
        const FString& ElementName,
        const FVector& CameraOffset,
        float BlendSeconds = 0.0f) const;

    bool FocusCameraOnSceneActorByName(
        const FString& ElementName,
        const FVector& CameraOffset,
        const FRotator& CameraRotator,
        float BlendSeconds) const;

    bool FocusCameraOnSceneActorGroup(
        const TArray<FString>& ElementNames,
        float BlendSeconds) const;

    bool FocusCameraOnCommanderGroup(
        const FString& CommanderName,
        float BlendSeconds) const;

    //-------------------------------------------------------------
    // Scene Actor Lookup
    //-------------------------------------------------------------
    AActor* FindSceneActorByName(const FString& ElementName) const;

    FString GetCommanderForElement(const FString& ElementName) const;
    int32 GetCommanderGroupActorCount(const FString& CommanderName) const;

    //-------------------------------------------------------------
    // Coordinate Conversion
    //-------------------------------------------------------------
    FVector ConvertLegacyRegionOffsetToSceneOffset(const FVector& LegacyOffset) const;
    FVector ConvertLegacyRegionOffsetToWorld(AActor* RegionActor, const FVector& LegacyOffset) const;

    float ConvertLegacyRegionSpeedToSceneSpeed(float LegacySpeed) const;

protected:
    //-------------------------------------------------------------
    // Resolver Helpers
    //-------------------------------------------------------------
    ASystemSceneBuilder* ResolveSystemSceneBuilder() const;

    bool ResolveRegionAnchorLocation(
        const FString& RegionName,
        FVector& OutWorldLocation) const;

    bool ResolveRegionCenterLocation(
        const FString& RegionName,
        FVector& OutWorldLocation) const;

    AActor* FindRegionActorByName(const FString& RegionName) const;

    //-------------------------------------------------------------
    // Location Conversion
    //-------------------------------------------------------------
    FVector ConvertLegacySceneLocToWorld(const FVector& LegacyLoc) const;
    FVector ConvertMissionElementLocToWorld(const FS_MissionElement& Elem) const;

    //-------------------------------------------------------------
    // Asset / Spawn Helpers
    //-------------------------------------------------------------
    FString ResolveModelNameForDesign(const FString& DesignName) const;
    FString ResolveMeshPathForDesign(const FString& DesignName) const;
    UStaticMesh* ResolveStaticMeshFromPath(const FString& MeshPath) const;

    AActor* SpawnSceneElementActor(
        const FString& ElementName,
        const FString& DesignName,
        const FVector& WorldLocation,
        int32 HeadingDegrees);

    //-------------------------------------------------------------
    // Runtime Ship Integration
    //-------------------------------------------------------------
    Ship* CreateRuntimeShipForMissionElement(
        const FS_MissionElement& Elem,
        const FVector& InitialWorldLocation);

    void RegisterRuntimeShipForElement(
        const FS_MissionElement& Elem,
        Ship* RuntimeShip,
        AShipActor* ShipActor);

    void LinkRuntimeShipCommanders(const TArray<FS_MissionElement>& Elements);

protected:
    //-------------------------------------------------------------
    // Components
    //-------------------------------------------------------------
    UPROPERTY()
    USceneComponent* SceneRoot = nullptr;

    //-------------------------------------------------------------
    // Actor Classes
    //-------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Scene")
    TSubclassOf<ASceneMeshActor> DefaultSceneMeshActorClass;

    //-------------------------------------------------------------
    // Scene Settings
    //-------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Scene")
    FVector SceneOriginOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Scene")
    float LegacyUnitsPerKm = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Scale")
    float MissionRegionOffsetScale = 8300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|MissionElements")
    float MissionElementScaleMultiplier = 1.0f;

    //-------------------------------------------------------------
    // Ship / Camera Scaling
    //-------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Scaling")
    float ShipLengthToSceneUnits = 0.01f;

    //-------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bEnableDebugLogs = true;

    //-------------------------------------------------------------
    // Spawned Scene State
    //-------------------------------------------------------------
    UPROPERTY()
    TArray<AActor*> OwnedActors;

    UPROPERTY()
    TArray<FCampaignSceneSpawnedActor> SpawnedSceneActors;

private:
    //-------------------------------------------------------------
    // Runtime Ship State
    //-------------------------------------------------------------
    TArray<Ship*> RuntimeShips;

    TMap<FString, Ship*> RuntimeShipByElementName;
    TMap<FString, AShipActor*> ShipActorByElementName;
};