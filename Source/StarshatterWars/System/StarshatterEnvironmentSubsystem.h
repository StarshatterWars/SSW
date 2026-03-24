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
        - Star systems (FS_StarSystem)
        - Stars (FS_Star)
        - Planets (FS_Planet)
        - Moons (FS_Moon)
        - Regions (FS_Region)
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

#include "StarshatterEnvironmentSubsystem.generated.h"

// Forward declarations
class DataLoader;
class AStarSystem;

class USSWGameInstance;

// Logging
DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterEnvironment, Log, All);

UCLASS()
class STARSHATTERWARS_API UStarshatterEnvironmentSubsystem
    : public UGameInstanceSubsystem
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

    // -----------------------------------------------------------------
    // Primary entry point
    // -----------------------------------------------------------------
    void LoadAll(bool bFull = false);

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

    void ParseStar(TermStruct* Val, const char* Fn);
    void ParsePlanet(TermStruct* Val, const char* Fn);
    void ParseMoon(TermStruct* Val, const char* Fn);
    void ParseRegion(TermStruct* Val, const char* Fn);

    void ParseStarMap(TermStruct* Val, const char* Fn);
    void ParsePlanetMap(TermStruct* Val, const char* Fn);
    void ParseMoonMap(TermStruct* Val, const char* Fn);

    void ParseTerrain(TermStruct* Val, const char* Fn);

    // -----------------------------------------------------------------
    // Star systems
    // -----------------------------------------------------------------
    void LoadStarsystems();
    void ParseStarSystem(const char* FileName);

    // -----------------------------------------------------------------
    // DataTable creation and export
    // -----------------------------------------------------------------
    void CreateEnvironmentTables();

    // -----------------------------------------------------------------
    // Lifetime control
    // -----------------------------------------------------------------
    void Unload();
    void Clear();

    bool IsLoaded() const { return bLoaded; }

    // -----------------------------------------------------------------
    // DataTable accessors
    // -----------------------------------------------------------------
    UDataTable* GetGalaxyTable() const { return GalaxyDataTable; }
    UDataTable* GetStarSystemsTable() const { return StarSystemDataTable; }
    UDataTable* GetStarsTable() const { return StarsDataTable; }
    UDataTable* GetPlanetsTable() const { return PlanetsDataTable; }
    UDataTable* GetMoonsTable() const { return MoonsDataTable; }
    UDataTable* GetRegionsTable() const { return RegionsDataTable; }
    UDataTable* GetTerrainRegionsTable() const { return TerrainRegionsDataTable; }
    UDataTable* GetZonesTable() const { return ZonesDataTable; }

    // -----------------------------------------------------------------
    // Public static data arrays
    // -----------------------------------------------------------------
    UPROPERTY()
    TArray<FS_Galaxy> GalaxyDataArray;

    UPROPERTY()
    TArray<FS_StarSystem> StarSystemDataArray;

    UPROPERTY()
    TArray<FS_Star> StarDataArray;

    UPROPERTY()
    TArray<FS_Planet> PlanetDataArray;

    UPROPERTY()
    TArray<FS_Moon> MoonDataArray;

    UPROPERTY()
    TArray<FS_Region> RegionDataArray;

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

    UDataTable* StarSystemDataTable = nullptr;
    UDataTable* StarsDataTable = nullptr;
    UDataTable* PlanetsDataTable = nullptr;
    UDataTable* MoonsDataTable = nullptr;

    UDataTable* TerrainRegionsDataTable = nullptr;
    UDataTable* ZonesDataTable = nullptr;

    // -----------------------------------------------------------------
    // Working row scratch
    // -----------------------------------------------------------------
    FS_Galaxy         GalaxyData;
    FS_StarSystem     StarSystemData;
    FS_Star           StarData;
    FS_Planet         PlanetData;
    FS_Moon           MoonData;
    FS_Region         RegionData;
    FS_TerrainRegion  TerrainRegionData;

    // Map format arrays
    TArray<FS_StarMap>   StarMapArray;
    TArray<FS_PlanetMap> PlanetMapArray;
    TArray<FS_MoonMap>   MoonMapArray;
    TArray<FS_RegionMap> RegionMapArray;

    // Legacy system registry
    List<AStarSystem> systems;

    // Paths
    FString ProjectPath;
    FString FilePath;

    private:
        // Cached GI (kept)
        USSWGameInstance* SSWInstance = nullptr;

            // ==================================================================== =
            // DT -> runtime hydration
            // =====================================================================
            void HydrateAllFromTables();

        void ReadGalaxyDataTable();
        void ReadStarSystemsTable();
        void ReadStarsTable();
        void ReadPlanetsTable();
        void ReadMoonsTable();
        void ReadRegionsTable();
        void ReadTerrainRegionsTable();

        void BuildEnvironmentCaches();

        void ClearRuntimeCaches();

        // =====================================================================
        // Lookup caches
        // =====================================================================
        UPROPERTY()
        TMap<FString, FS_Galaxy> GalaxyByName;

        UPROPERTY()
        TMap<FString, FS_StarSystem> StarSystemByName;

        UPROPERTY()
        TMap<FString, FS_Star> StarByName;

        UPROPERTY()
        TMap<FString, FS_Planet> PlanetByName;

        UPROPERTY()
        TMap<FString, FS_Moon> MoonByName;

        UPROPERTY()
        TMap<FString, FS_Region> RegionByName;

        UPROPERTY()
        TMap<FString, FS_TerrainRegion> TerrainRegionByName;

        UPROPERTY()
        TMap<FString, FString> RegionParentByName;
        TMap<FString, TArray<FString>> RegionChildrenByParent;
};

