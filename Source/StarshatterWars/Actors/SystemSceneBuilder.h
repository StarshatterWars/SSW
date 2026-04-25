#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameStructs.h"
#include "SystemSceneBuilder.generated.h"

class USceneComponent;
class UStarshatterEnvironmentSubsystem;
class StarSystem;
class OrbitalBody;
class OrbitalRegion;
class ACameraActor;
class ASkyLight;

UENUM(BlueprintType)
enum class ESystemSceneSource : uint8
{
    StarSystem UMETA(DisplayName = "Star System"),
    Galaxy     UMETA(DisplayName = "Galaxy")
};

UENUM(BlueprintType)
enum class ESystemSceneScaleMode : uint8
{
    PreviewCompressed UMETA(DisplayName = "Preview Compressed"),
    LegacyCinematic   UMETA(DisplayName = "Legacy Cinematic")
};

USTRUCT(BlueprintType)
struct FSystemSceneScaleSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
    ESystemSceneScaleMode ScaleMode = ESystemSceneScaleMode::LegacyCinematic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    bool bUseLogOrbitScaling = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float OrbitUnitsPerMillionKm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float RadiusUnitsPerThousandKm = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MinOrbitUnits = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MaxOrbitUnits = 12000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MinBodyScaleUnits = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MaxBodyScaleUnits = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MinOrbitKm = 50000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Preview")
    float MaxOrbitKm = 8000000000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Legacy")
    float LegacyUnitsPerKm = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Legacy")
    float LegacyMoonUnitsPerKm = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Legacy")
    float LegacyMaxOrbitUnits = 500000000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|LegacyCamera")
    float LegacyPlanetCameraFactor = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
    float MoonOrbitMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
    float VerticalOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MinStarScaleUnits = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MaxStarScaleUnits = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MinPlanetScaleUnits = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MaxPlanetScaleUnits = 1000.00f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MinMoonScaleUnits = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MaxMoonScaleUnits = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MinGasGiantScaleUnits = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float MaxGasGiantScaleUnits = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale|Bodies")
    float GasGiantScaleMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FSpawnedSystemBody
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString BodyName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> Actor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> ParentActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsMoon = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsOrbit = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float VisualRadiusUnits = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float OrbitRadiusUnits = 0.0f;
};

USTRUCT(BlueprintType)
struct FSpawnedSystemRegion
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString RegionName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString AnchorBodyName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> Actor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<AActor> ParentActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float InnerRadiusUnits = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float OuterRadiusUnits = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float GridUnits = 0.0f;
};

UCLASS()
class STARSHATTERWARS_API ASystemSceneBuilder : public AActor
{
    GENERATED_BODY()

public:
    ASystemSceneBuilder();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    void BuildSystemScene();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    void ClearSpawnedBodies();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    void RefreshRuntimeBodyTransforms();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    bool FocusCameraOnBodyByName(
        const FString& BodyName,
        const FVector& CameraOffset,
        float BlendSeconds = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    bool GetBodyWorldLocationByName(
        const FString& BodyName,
        FVector& OutWorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    bool ApplyCameraViewVector(
        const FVector& ViewVector,
        float BlendSeconds = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    bool DebugFindBodyByName(const FString& BodyName) const;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|SystemScene")
    bool DebugFocusCameraOnBodyByName(
        const FString& BodyName,
        const FVector& CameraOffset);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemScene")
    const TArray<FSpawnedSystemBody>& GetSpawnedBodies() const
    {
        return SpawnedBodies;
    }

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemScene")
    const TArray<FSpawnedSystemRegion>& GetSpawnedRegions() const
    {
        return SpawnedRegions;
    }

    void SetTemporaryCutsceneBodyScale(
        const FString& BodyName,
        float ScaleMultiplier);

    bool GetRegionByName(
        const FString& RegionName,
        FSpawnedSystemRegion& OutRegion) const;

    void SpawnDirectionalLight();
    void SpawnSkyLight();

    bool GetRegionWorldLocationByName(
        const FString& RegionName,
        FVector& OutWorldLocation) const;

    void ResetTemporaryCutsceneBodyScales();

    bool FindSpawnedBodyByName(
        const FString& BodyName,
        FSpawnedSystemBody& OutBody) const;

    float ConvertOrbitKmToSceneUnits(float OrbitKm, bool bIsMoonOrbit = false) const;

protected:
    bool ResolveStarSystemRow(FStarSystem& OutRow) const;
    bool ResolveGalaxyRow(FS_Galaxy& OutRow) const;

    StarSystem* ResolveRuntimeSystem() const;
    OrbitalBody* ResolvePrimaryStar(StarSystem* RuntimeSystem) const;

    void BuildFromRuntimeSystem(StarSystem* RuntimeSystem);

    void BuildRuntimeStar(
        OrbitalBody* StarBody,
        const FVector& StarAnchorWorldLocation,
        AActor* ParentActor,
        AActor*& OutStarActor);

    void BuildRuntimePlanet(
        OrbitalBody* PlanetBody,
        const FVector& StarAnchorWorldLocation,
        const FVector& PrimaryStarRuntimeLocation,
        AActor* ParentActor);

    void BuildRuntimeMoon(
        OrbitalBody* MoonBody,
        const FVector& StarAnchorWorldLocation,
        const FVector& PrimaryStarRuntimeLocation,
        AActor* ParentActor);

    void BuildRuntimeRegionForBody(
        const FString& BodyName,
        const FVector& BodyWorldLocation,
        AActor* BodyActor,
        float BodyVisualRadiusUnits,
        bool bIsMoon);

    void TrackRuntimeBody(
        const FString& BodyName,
        OrbitalBody* Body,
        AActor* Actor);



    TSubclassOf<AActor> ResolvePlanetActorClass(EPlanetType PlanetType, bool bIsMoon) const;
    EPlanetType ResolvePlanetTypeFromData(const FString& BodyName, bool bIsMoon) const;

    float ConvertStarRadiusKmToSceneUnits(float RadiusKm) const;
    float ConvertPlanetRadiusKmToSceneUnits(float RadiusKm) const;
    float ConvertMoonRadiusKmToSceneUnits(float RadiusKm) const;
    float ConvertGasGiantRadiusKmToSceneUnits(float RadiusKm) const;

    
    float ConvertRadiusKmToSceneUnits(float RadiusKm) const;

    FVector ConvertRuntimeOffsetToSceneOffset(
        const FVector& RuntimeOffsetKm,
        bool bIsMoon) const;

    FVector ConvertLegacyCameraOffsetToSceneOffset(const FVector& LegacyOffset) const;

    AActor* SpawnBodyActor(
        TSubclassOf<AActor> BodyClass,
        const FString& BodyName,
        const FVector& WorldLocation,
        float VisualRadiusUnits,
        AActor* ParentActor,
        bool bIsMoon,
        bool bIsStar);

    AActor* SpawnOrbitActor(
        const FString& OrbitName,
        const FVector& WorldLocation,
        float OrbitRadiusUnits,
        AActor* ParentActor);

    AActor* SpawnRegionActor(
        const FString& RegionName,
        const FVector& WorldLocation,
        float OuterRadiusUnits,
        AActor* ParentActor);

    void RegisterSpawnedRegion(
        const FString& RegionName,
        const FString& AnchorBodyName,
        AActor* Actor,
        AActor* ParentActor,
        const FVector& SpawnLocation,
        float InnerRadiusUnits,
        float OuterRadiusUnits,
        float GridUnits);

    void RegisterSpawnedBody(
        const FString& BodyName,
        AActor* Actor,
        AActor* ParentActor,
        bool bIsMoon,
        bool bIsOrbit,
        const FVector& SpawnLocation,
        float VisualRadiusUnits,
        float OrbitRadiusUnits);

    UStarshatterEnvironmentSubsystem* GetEnvironmentSubsystem() const;

    void FocusPlayerCameraOnSystem(const FVector& FocusPoint, float Distance) const;
    void FocusPlayerCameraOnSpawnedBodies() const;

    void LogRuntimeSystemSummary(StarSystem* RuntimeSystem) const;
    void LogTrackedBodies(const TCHAR* Label) const;


public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bBuildOnBeginPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bRebuildInEditor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    ESystemSceneSource DataSource = ESystemSceneSource::Galaxy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    FString TargetSystemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    FString TargetGalaxyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    FSystemSceneScaleSettings ScaleSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bSpawnOrbitActors = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bSpawnRegionActors = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bDestroyPreviousBodiesOnBuild = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Regions")
    float DefaultRegionRadiusKm = 480000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Regions")
    float DefaultRegionGridKm = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Debug")
    bool bEnableDebugLogs = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Debug")
    bool bLogEveryTick = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Debug")
    float DebugLogIntervalSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System")
    bool bAnimateBodies = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<AActor> StarActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<AActor> PlanetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<AActor> MoonActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<AActor> OrbitActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSubclassOf<AActor> RegionActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Planet Types")
    TSubclassOf<AActor> TerranPlanetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Planet Types")
    TSubclassOf<AActor> IcePlanetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Planet Types")
    TSubclassOf<AActor> VolcanicPlanetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Planet Types")
    TSubclassOf<AActor> BarrenPlanetActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Planet Types")
    TSubclassOf<AActor> GasGiantPlanetActorClass;




protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    FString LastResolvedName;

    UPROPERTY()
    ASkyLight* SceneSkyLight = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    TArray<FSpawnedSystemBody> SpawnedBodies;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    TArray<FSpawnedSystemRegion> SpawnedRegions;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedActors;

    UPROPERTY()
    TObjectPtr<ACameraActor> SceneCameraActor = nullptr;

    TMap<FString, OrbitalBody*> RuntimeBodyMap;
    TMap<FString, TObjectPtr<AActor>> BodyActorMap;

    float DebugTickAccumulator = 0.0f;
    int32 TickCounter = 0;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<AActor>, FVector> OriginalCutsceneBodyScales;
};