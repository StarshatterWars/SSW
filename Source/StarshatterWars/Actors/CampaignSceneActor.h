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
class SimObject;

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
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
        const FRotator& CameraRotator,
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

    void LinkRuntimeShipCommanders(const TArray<FS_MissionElement>& Elements);

protected:
    UPROPERTY(EditAnywhere, Category = "Runtime AI")
    bool bEnableRuntimeAITick = true;

    UPROPERTY(EditAnywhere, Category = "Runtime AI")
    float RuntimeAITimeScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Runtime AI")
    float MaxRuntimeTickSeconds = 0.05f;

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

private:
    FVector GetFormationOffsetForElement(
        const FS_MissionElement& Elem,
        int32 FollowerIndex,
        int32 FollowerCount) const;

    void BuildSimRegionsFromEnvironment();
    
    void ApplyRuntimeFormationOffsets(const TArray<FS_MissionElement>& Elements);

    void TickRuntimeShips(float DeltaSeconds);
    void ClearRuntimeShips();

private:
    Ship* CurrentPlayerShip = nullptr;
    FString CurrentMissionRegionName;

};