/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           StarshatterEnvironmentSubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Authoritative environment data loader and registry owner.

    UStarshatterEnvironmentSubsystem is responsible for loading, parsing,
    normalizing, and caching all static environment data required at runtime:

        - Galaxy definitions (FS_Galaxy)
        - Star systems (FStarSystem)
        - Stars (FStarystem)
        - Planets (FPlanet)
        - Moons (FMoon)
        - Regions (FRegion)
        - Terrain regions (FS_TerrainRegion)
        - Campaign zones (FS_CampaignZone)

    The subsystem has no world presence and does not tick.
    It exists purely as a service and static data registry.

    RESPONSIBILITIES
    ================
    - Load and parse legacy environment definitions
    - Populate Unreal DataTables
    - Build in-memory arrays for UI and runtime queries
    - Resolve parent-child relationships:
        System -> Star -> Planet -> Moon
        System -> Region graph
    - Provide DT-first read-only accessors

    NON-GOALS
    =========
    - No Actor spawning
    - No Tick()
    - No runtime orbital simulation
    - No UI logic
    - No player/session state

    This subsystem is STATIC ENVIRONMENT DATA ONLY.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// Legacy parsing headers
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Random.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"

// Engine helpers
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"
#include "HAL/FileManagerGeneric.h"

// Project types
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "Tickable.h"

#include "StarshatterEnvironmentSubsystem.generated.h"

// Forward declarations
class DataLoader;
class StarSystem;

class OrbitalBody;
class OrbitalRegion;

class USSWGameInstance;

// Logging
DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterEnvironment, Log, All);

UCLASS()
class STARSHATTERWARS_API UStarshatterEnvironmentSubsystem
    : public UGameInstanceSubsystem
    , public FTickableGameObject
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------
    // Subsystem lifecycle
    // -----------------------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void ReleaseAssets();

    void GetSSWInstance();

public:
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    void BuildRuntimeStarSystems();
    virtual bool IsTickableInEditor() const override { return false; }
    virtual bool IsTickableWhenPaused() const override { return false; }

    const TArray<StarSystem*>& GetRuntimeStarSystems() const { return RuntimeStarSystems; }

    const FS_Galaxy* FindGalaxyByName(const FString& InName) const;
    const FStarSystem* FindStarSystemByName(const FString& InName) const;

    const FPlanet* FindPlanetMapByName(const FString& Name) const;
    const FMoon* FindMoonMapByName(const FString& Name) const;

    // -----------------------------------------------------------------
    // Primary entry point
    // -----------------------------------------------------------------
    void LoadAll(bool bFull = false);

    // ===================================================================== 
    // Runtime Simulation Time System 
    // ===================================================================== 
    void InitSimulationBaseTime();

    void RegisterStarSystem(StarSystem* System);

    void RegisterStar(OrbitalBody* Body);
    void RegisterPlanet(OrbitalBody* Body);
    void RegisterMoon(OrbitalBody* Body);
    void RegisterRegion(OrbitalRegion* Region);

    double GetEnvironmentBaseTime() const { return EnvironmentBaseTime; }
    bool IsBaseTimeInitialized() const { return bBaseTimeInitialized; }
    void TickEnvironmentTime(double DeltaSeconds);

    void ResetSimulationClock();
    void SetSimulationClockMs(int64 InMs);
    void AdvanceSimulationClock(double DeltaSeconds);

    int64 GetSimulationClockMs() const { return SimulationClockMs; }
    double GetSimulationClockSeconds() const { return (double)SimulationClockMs / 1000.0; }

    EGameMode GetGameMode() { return game_mode; }
    void      SetGameMode(EGameMode mode) { game_mode = mode; }

    // =====================================================================
    // Project path / utility
    // =====================================================================
    void SetProjectPath();
    FString GetProjectPath();

    bool GetRegionTypeFromString(const FString& InString, EOrbitalType& OutValue);

    // -----------------------------------------------------------------
    // Galaxy and environment parsing
    // -----------------------------------------------------------------
    void LoadGalaxyMap();

    void ParseRegion(TermStruct* Val, const char* Fn);

    void ParseStarMap(TermStruct* Val, const char* Fn);
    void ParsePlanetMap(TermStruct* Val, const char* Fn);
    void ParseMoonMap(TermStruct* Val, const char* Fn);

    void ParseTerrain(TermStruct* Val, const char* Fn);

    // -----------------------------------------------------------------
    // DataTable creation and export
    // -----------------------------------------------------------------
    void CreateEnvironmentTables();

    void ResolveDataTables();

    // -----------------------------------------------------------------
    // Lifetime control
    // -----------------------------------------------------------------
    void Unload();
    void Clear();

    bool IsLoaded() const { return bLoaded; }

    UPROPERTY()
    TMap<FString, FPlanet> PlanetMapByName;

    UPROPERTY()
    TMap<FString, FMoon> MoonMapByName;

    // -----------------------------------------------------------------
    // DataTable accessors
    // -----------------------------------------------------------------
    UDataTable* GetGalaxyTable() const { return GalaxyDataTable; }
    UDataTable* GetStarsTable() const { return StarsDataTable; }
    UDataTable* GetPlanetsTable() const { return PlanetsDataTable; }
    UDataTable* GetMoonsTable() const { return MoonsDataTable; }
    UDataTable* GetRegionsTable() const { return RegionsDataTable; }
    UDataTable* GetTerrainRegionsTable() const { return TerrainRegionsDataTable; }
    UDataTable* GetZonesTable() const { return ZonesDataTable; }
    
    static UStarshatterEnvironmentSubsystem* Get();

    // -----------------------------------------------------------------
    // Public static data arrays
    // -----------------------------------------------------------------
    UPROPERTY()
    TArray<FS_Galaxy> GalaxyDataArray;

    UPROPERTY()
    TArray<FStarSystem> StarSystemDataArray;

    UPROPERTY()
    TArray<FStarSystem> StarDataArray;

    UPROPERTY()
    TArray<FPlanet> PlanetDataArray;

    UPROPERTY()
    TArray<FMoon> MoonDataArray;

    UPROPERTY()
    TArray<FRegion> RegionDataArray;

    UPROPERTY()
    TArray<FS_TerrainRegion> TerrainRegionsArray;

    UPROPERTY()
    TArray<FS_CampaignZone> ZoneDataArray;

protected:

    // -----------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------
    bool bLoaded = false;

    // -----------------------------------------------------------------
    // DataTables
    // -----------------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|Environment|DataTables")
    TObjectPtr<UDataTable> GalaxyDataTable = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Starshatter|Environment|DataTables")
    TObjectPtr<UDataTable> RegionsDataTable = nullptr;

    UDataTable* StarsDataTable = nullptr;
    UDataTable* PlanetsDataTable = nullptr;
    UDataTable* MoonsDataTable = nullptr;

    UDataTable* TerrainRegionsDataTable = nullptr;
    UDataTable* ZonesDataTable = nullptr;

    // -----------------------------------------------------------------
    // Working row scratch
    // -----------------------------------------------------------------
    FS_Galaxy         GalaxyData;
    FStarSystem       StarSystemData;
    FStarSystem       StarData;
    FPlanet           PlanetData;
    FMoon             MoonData;
    FRegion           RegionData;
    FS_TerrainRegion  TerrainRegionData;

    // Map format arrays
    TArray<FStarSystem>  StarMapArray;
    TArray<FPlanet>      PlanetMapArray;
    TArray<FMoon>        MoonMapArray;
    TArray<FRegion>      RegionMapArray;

    // Paths
    FString ProjectPath;
    FString FilePath;

    // ===================================================================== 
    // Runtime Simulation Time System 
    // ===================================================================== 

    bool bBaseTimeInitialized = false;
    double EnvironmentBaseTime = 0.0;
    int64 SimulationClockMs = 0;

    TArray<StarSystem*>    RuntimeStarSystems;
    TArray<OrbitalBody*>   RuntimeStars;
    TArray<OrbitalBody*>   RuntimePlanets;
    TArray<OrbitalBody*>   RuntimeMoons;
    TArray<OrbitalRegion*> RuntimeRegions;

private:
    // Cached GI (kept)
    USSWGameInstance* SSWInstance = nullptr;

    // ==================================================================== =
    // DT -> runtime hydration
    // =====================================================================
    bool HydrateAllFromTables();

    bool ReadGalaxyDataTable();
    bool BuildStarSystemArrayFromGalaxy();
    bool ReadStarsTable();
    bool ReadPlanetsTable();
    bool ReadMoonsTable();
    bool ReadRegionsTable();
    bool ReadTerrainRegionsTable();

    void BuildEnvironmentCaches();

    void ClearRuntimeCaches();

    // =====================================================================
    // Lookup caches
    // =====================================================================
    UPROPERTY()
    TMap<FString, FS_Galaxy> GalaxyByName;

    UPROPERTY()
    TMap<FString, FStarSystem> StarSystemByName;

    UPROPERTY()
    TMap<FString, FStarSystem> StarByName;

    UPROPERTY()
    TMap<FString, FPlanet> PlanetByName;

    UPROPERTY()
    TMap<FString, FMoon> MoonByName;

    UPROPERTY()
    TMap<FString, FRegion> RegionByName;

    UPROPERTY()
    TMap<FString, FS_TerrainRegion> TerrainRegionByName;

    UPROPERTY()
    TMap<FString, FString> RegionParentByName;
    TMap<FString, TArray<FString>> RegionChildrenByParent;
    
    static TWeakObjectPtr<UStarshatterEnvironmentSubsystem> ActiveInstance;

    EGameMode               game_mode;
};

