/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           StarshatterGameDataSubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Global game data loader and registry owner.

    This subsystem replaces the legacy AGameDataLoader actor.
    It is responsible for loading, parsing, and caching all
    static and semi-static game data required at runtime.

    The subsystem has no world presence and does not tick.
    It exists purely as a service and data registry.

    RESPONSIBILITIES
    ================
    - Load and parse legacy data files (cfg, def, script, text)
    - Generate and populate Unreal DataTables
    - Build registries for:
        * Galaxy and star systems
        * Campaigns and order-of-battle data
        * Missions, templates, and scripted scenarios
        * Ship, system, and component designs
        * Awards, forms, and UI layout definitions
    - Provide read-only access to loaded data (DT-first)

    NON-GOALS
    =========
    - No Actor spawning
    - No Tick()
    - No Transform or spatial behavior
    - No UI or presentation logic
    - No player persistence
    - No runtime “active campaign” state

    RUNTIME SEPARATION
    ==================
    Player save + active session state belong in:
      - UStarshatterPlayerSubsystem
      - UStarshatterDataRuntimeSubsystem

    This subsystem is STATIC DATA ONLY.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// Legacy parsing / registry headers (kept for now)
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Random.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"
#include "GameLoader.h"

// Engine / file helpers
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"
#include "HAL/FileManagerGeneric.h"

// Project types
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "Tickable.h"

#include "StarshatterGameDataSubsystem.generated.h"

// ---------------------------------------------------------------------
// Forward declarations (legacy)
// ---------------------------------------------------------------------
class Campaign;
class CampaignPlan;
class Combatant;
class CombatAction;
class CombatEvent;
class CombatGroup;
class CombatUnit;
class CombatZone;
class DataLoader;
class Mission;
class MissionTemplate;
class TemplateList;
class MissionInfo;
class AStarSystem;
class SystemDesign;
class ComponentDesign;

// Cached GI (optional)
class USSWGameInstance;

DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterGameData, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogStarshatterGameDataCampaign, Log, All);

// ---------------------------------------------------------------------
// UStarshatterGameDataSubsystem
// ---------------------------------------------------------------------
UCLASS()
class STARSHATTERWARS_API UStarshatterGameDataSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // =====================================================================
    // Subsystem lifecycle
    // =====================================================================
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void GetSSWInstance();

    UDataTable* GetCampaignDataTable() const { return CampaignDataTable; }
    const TArray<FS_Campaign>& GetCampaignDataArray() const { return CampaignDataArray; }

    // =====================================================================
    // Primary entry point
    // =====================================================================
    // Called by GameInitSubsystem during EGameMode::INIT
    void LoadAll(bool bFull = false);

    // =====================================================================
    // Project path / utility
    // =====================================================================
    void SetProjectPath();
    FString GetProjectPath();

    bool GetRegionTypeFromString(const FString& InString, EOrbitalType& OutValue);


    // =====================================================================
    // Campaigns (static load + DT hydration)
    // =====================================================================
    void LoadCampaignData(const char* FileName, bool full = false);

    void ReadCombatRosterData();

    // DT hydration methods (static -> DT/arrays)
    void ReadCampaignData();

    void ReadOrderOfBattleData();

    const FS_OOBForce* GetForceById(int32 ForceId) const;
    const FS_Campaign* GetCampaignByIndex1Based(int32 CampaignIndex1Based) const;

   // =====================================================================
    // Missions / templates
    // =====================================================================
    void LoadZones(
        const FString& Path,
        const FString& CampaignId);

    void LoadMissionList(FString Path);
    void LoadTemplateList(FString Path);

    void LoadMission(FString Name);
    void LoadTemplateMission(FString Name);
    void LoadScriptedMission(FString Name);

    void BuildCombatRosterFromOrderOfBattle();

    // Mission parsing
    void ParseMission(const char* filename);
    void ParseNavpoint(TermStruct* val, const char* fn);
    void ParseObjective(TermStruct* val, const char* fn);
    void ParseInstruction(TermStruct* val, const char* fn);
    void ParseShip(TermStruct* val, const char* fn);
    void ParseMissionLoadout(TermStruct* val, const char* fn);
    void ParseEvent(TermStruct* val, const char* fn);
    void ParseElement(TermStruct* val, const char* fn);
    void ParseScriptedTemplate(const char* fn);
    void ParseMissionTemplate(const char* fname);
    void ParseAlias(TermStruct* val, const char* fn);
    void ParseRLoc(TermStruct* val, const char* fn);
    void ParseCallsign(TermStruct* val, const char* fn);
    void ParseOptional(TermStruct* val, const char* fn);


    // =====================================================================
    // Designs / OOB
    // =====================================================================

    void InitializeCampaignData();
    void InitializeCombatRoster();

    void LoadCombatRoster(const char* InFilename, int32 Team);

    void ParseCombatUnit();

    // OOB helpers
    CombatGroup* CloneOver(CombatGroup* force, CombatGroup* clone, CombatGroup* group);

    void InitializeOrderOfBattleTable();
    void ExportDataToCSV(UDataTable* DataTable, const FString& FileName);

    // =====================================================================
    // Content bundles / forms / awards
    // =====================================================================
    Text GetContentBundleText(const char* key) const;

    void LoadContentBundle();
    void LoadAwardTables();

    bool GetRankInfo(int32 RankId, FRankInfo& Out) const;
    bool GetMedalInfo(int32 MedalId, FMedalInfo& Out) const;

    bool GetMedalInfoByFlag(uint32 MedalFlag, FMedalInfo& OutRow) const;

    void BuildMedalCache_ByFlag(const UDataTable* MedalsTable);

    bool FillRankInfoFromTable(const UDataTable* RanksTable, int32 RankId, FRankInfo& OutRank);
    bool FillMedalInfoFromTable(const UDataTable* MedalsTable, int32 MedalId, FMedalInfo& OutMedal);

    UFUNCTION(BlueprintPure, Category = "Starshatter|Awards")
    UDataTable* GetRanksTable() const { return DT_Ranks; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|Awards")
    UDataTable* GetMedalsTable() const { return DT_Medals; }

    // Optional: unified view
    UFUNCTION(BlueprintPure, Category = "Starshatter|Awards")
    UDataTable* GetAllAwardsTable() const { return DT_AwardsAll; }
    
    TMap<int32, FRankInfo> RankById;
    TMap<int32, FMedalInfo> MedalById;
    TMap<uint32, FMedalInfo> MedalByFlag;

    // =====================================================================
    // Lifetime / state
    // =====================================================================
    void Unload();
    void Clear();

    void SetCampaignStatus(ECampaignStatus s);
    double Stardate();

    Combatant* GetCombatant(const char* cname);

    bool IsContentBundleLoaded() const { return !ContentValues.IsEmpty(); }
    bool IsLoaded() const { return bLoaded; }

    // =====================================================================
    // Public read lists used by UI (static data)
    // =====================================================================
    UPROPERTY()
    TArray<FS_CampaignMissionList> MissionList;

    UPROPERTY()
    TArray<FS_OOBForce> AllForces;

    UPROPERTY()
    TMap<FName, FS_OOBForce> ForceMap;

    // Optional (recommended)
    TMap<int32, int32> ForceIdLookup;

public:
        virtual void Tick(float DeltaTime) override;
        virtual bool IsTickable() const override;
        virtual TStatId GetStatId() const override;

private:
    // =====================================================================
    // Internal helpers
    // =====================================================================
    void CacheSSWInstance();

private:
    // =====================================================================
    // Internal state flags
    // =====================================================================
    bool bLoaded = false;

public:
    // =====================================================================
    // Legacy fields (kept intact; grouped for readability)
    // =====================================================================

    Text                 ContentName;
    Dictionary<Text>     ContentValues;

    // Campaign header / metadata scratch
    int                  campaign_id;
    ECampaignStatus      CampaignStatus;
    int                  CampaignIndex;
    FString              CurrentCampaignId;
    char                 filename[64];
    Text                 path[64];
    Text                 name;
    Text                 description;
    Text                 situation;
    Text                 system;
    Text                 region;
    Text                 start;
    Text                 MainImage;
    Text                 orders;

    bool                 scripted;
    bool                 available;
    bool                 sequential;
    bool                 loaded_from_savegame;

    // Legacy registries
    List<Combatant>      combatants;
    List<AStarSystem>    systems;
    List<CombatZone>     zones;
    List<CampaignPlan>   planners;
    List<MissionInfo>    missions;
    List<TemplateList>   templates;
    List<CombatAction>   actions;
    List<CombatEvent>    events;

    bool                 bClearTables;
    
    TMap<int32, const FS_Campaign*> CampaignLookup;

    UPROPERTY(Transient)
    bool bAwardTablesLoaded = false;

    const TArray<FS_Campaign>& GetAllCampaigns() const { return AllCampaigns; }

    const FS_Campaign* GetCampaignByRow(FName RowName) const
    {
        return CampaignMap.Find(RowName);
    }

    const FS_Campaign* GetCampaignByIndex(int32 Index) const
    {
        return AllCampaigns.IsValidIndex(Index) ? &AllCampaigns[Index] : nullptr;
    }

    void SetActiveCampaign(const FS_Campaign& Campaign);

    FS_Campaign GetActiveCampaign() const { return ActiveCampaign; }

protected:
    CombatGroup* BuildCombatForceTree(const FS_OOBForce& ForceRow);

    void AddFleetToForce(CombatGroup* ForceGroup, const FS_OOBFleet& FleetRow);
    void AddCarrierGroupToFleet(CombatGroup* FleetGroup, const FS_OOBCarrier& CarrierRow);
    void AddDestroyerSquadronToFleet(CombatGroup* FleetGroup, const FS_OOBDestroyer& DestroyerRow);
    void AddBattleGroupToFleet(CombatGroup* FleetGroup, const FS_OOBBattle& BattleRow);

    void AddBattalionToForce(CombatGroup* ForceGroup, const FS_OOBBattalion& BattalionRow);
    void AddCivilianToForce(CombatGroup* ForceGroup, const FS_OOBCivilian& CivilianRow);

    void AddWingToCarrier(CombatGroup* CarrierGroup, const FS_OOBWing& WingRow);
    void AddInterceptSquadronToWing(CombatGroup* WingGroup, const FS_OOBIntercept& Row);
    void AddAttackSquadronToWing(CombatGroup* WingGroup, const FS_OOBAttack& Row);
    void AddFighterSquadronToWing(CombatGroup* WingGroup, const FS_OOBFighter& Row);
    void AddLandingSquadronToWing(CombatGroup* WingGroup, const FS_OOBLanding& Row);

    void AddBatteryToBattalion(CombatGroup* BattalionGroup, const FS_OOBBattery& Row);
    void AddStationToBattalion(CombatGroup* BattalionGroup, const FS_OOBStation& Row);
    void AddStarbaseToBattalion(CombatGroup* BattalionGroup, const FS_OOBStarbase& Row);
    void AddMinefieldToFleet(CombatGroup* FleetGroup, const FS_OOBMinefield& Row);

    void AddUnitsToCombatGroup(CombatGroup* Parent, const TArray<FS_OOBUnit>& Units);
    void AddUnitsToCombatGroup(CombatGroup* Parent, const TArray<FS_OOBFighterUnit>& Units);
    void AddUnitsToCombatGroup(CombatGroup* Parent, const TArray<FS_OOBMinefieldUnit>& Units);
    void AddUnitsToCombatGroup(CombatGroup* Parent, const TArray<FS_OOBStationUnit>& Units);
    void AddUnitsToCombatGroup(CombatGroup* Parent, const TArray<FS_OOBStarbaseUnit>& Units);

    // New OOB System
    void BuildCombatantsFromDataTables(const TMap<int32, CombatGroup*>& GroupById);
    void ReadCombatants();
    CombatGroup* BuildCombatForceFromRows(const TArray<FS_CombatGroup>& Rows, EEMPIRE_NAME Empire, Combatant* CombatantOwner);

    void BuildCombatantsFromData(const TArray<FS_Combatant>& CombatantRows, const TMap<int32, CombatGroup*>& GroupById, Campaign* CampaignPtr);
    void BuildCombatRosterFromDataTables();

    TMap<int32, CombatGroup*> BuildGroupMapFromDataTable();
    void LinkGroupHierarchy(const TArray<FS_CombatGroup>& InRows, TMap<int32, CombatGroup*>& GroupById);
    void BuildUnitsForGroups(const TArray<FS_CombatGroup>& InRows, TMap<int32, CombatGroup*>& GroupById);

    FString GetEmpireRosterName(EEMPIRE_NAME Empire) const;

protected:
    // =====================================================================
    // Parse scratch sizes / ids
    // =====================================================================
    int ActionSize;
    int CombatantSize;
    int GroupSize;

    Text  GroupType;
    int   GroupId;

    Text CombatantName;
    Text CombatantType;
    int CombatantId;

    // =====================================================================
    // DataTables (kept intact)
    // =====================================================================
    UDataTable* CampaignDataTable;
    UDataTable* CampaignActionDataTable;
    UDataTable* CombatGroupDataTable;
    UDataTable* OrderOfBattleDataTable;
    UDataTable* CampaignOOBDataTable;

    // =====================================================================
    // DT row scratch + arrays (kept intact)
    // =====================================================================
    FS_Combatant        NewCombatUnit;
    FS_CombatantGroup   NewGroupUnit;

    TArray<FS_Campaign>    CampaignDataArray;
    TArray<FS_CombatGroup> CombatRosterData;
    TArray<FS_Combatant>   CombatantData;
    TArray<FS_Galaxy>      GalaxyDataArray;
    TArray<TArray<uint8>> SystemDesignStringStorage;

    FS_Campaign        CampaignData;
    FShipDesign        ShipDesignData;

    FS_CombatGroupUnit CombatGroupUnit;
    FS_CombatGroup     CombatGroupData;
    FS_OOBForce        ForceData;

    // =====================================================================
    // Large working arrays (kept intact)
    // =====================================================================
    TArray<FS_OOBFleet> FleetArray;
    TArray<FS_CampaignAction> CampaignActionArray;
    TArray<FS_Combatant> CombatantArray;
    TArray<FS_CampaignZone> ZoneArray;
    TArray<FS_CampaignMissionList> MissionListArray;
    TArray<FS_CampaignTemplateList> TemplateListArray;
    TArray<FS_CampaignMission> MissionArray;
    TArray<FS_MissionElement> MissionElementArray;
    TArray<FS_MissionEvent> MissionEventArray;
    TArray<FS_MissionLoadout> MissionLoadoutArray;
    TArray<FS_MissionCallsign> MissionCallsignArray;
    TArray<FS_MissionOptional> MissionOptionalArray;
    TArray<FS_MissionAlias> MissionAliasArray;
    TArray<FS_MissionShip> MissionShipArray;
    TArray<FS_RLoc> MissionRLocArray;
    TArray<FS_CampaignReq> CampaignActionReqArray;
    TArray<FS_MissionInstruction> MissionInstructionArray;
    TArray<FS_MissionInstruction> MissionObjectiveArray;
    TArray<FS_MissionInstruction> MissionNavpointArray;

    TArray<FS_CombatGroupUnit> NewCombatUnitArray;

    TArray<FS_TemplateMission> TemplateMissionArray;
    TArray<FS_TemplateMission> ScriptedMissionArray;

    FS_CampaignAction NewCampaignAction;

    // DataTables (additional)
    UDataTable* ZonesDataTable;
    UDataTable* ShipDesignDataTable;
    UDataTable* SystemDesignDataTable;
    
    UDataTable* AwardsDataTable;
    UDataTable* RanksDataTable;
    UDataTable* MedalsDataTable;

	FS_Campaign ActiveCampaign;

    // CampaignAction parse scratch
    int      ActionId;
    Text     ActionType;
    int      ActionSubtype;
    int      OppType;
    int      ActionTeam;
    Text     ActionSource;
    FVector  ActionLocation;
    Text  ActionSystem;
    Text  ActionRegion;
    Text  ActionFile;
    Text  ActionImage;
    Text  ActionAudio;
    Text  ActionDate;
    Text  ActionScene;
    Text  ActionText;

    int   ActionCount;
    int   StartBefore;
    int   StartAfter;
    int   MinRank;
    int   MaxRank;
    int   Delay;
    int   Probability;

    Text  AssetType;
    int   AssetId;
    Text  TargetType;
    int   TargetId;
    int   TargetIff;

    Text  AssetKill;
    Text  TargetKill;

    Text ZoneRegion;
    Text ZoneSystem;

    int  Action;
    ECombatActionStatus ActionStatus;
    bool NotAction;

    Text Combatant1;
    Text Combatant2;

    int  comp;
    int  score;
    int  intel;
    int  gtype;
    int  gid;

    FString CampaignPath;

    // Unit scratch
    Text UnitName;
    Text UnitRegnum;
    Text UnitRegion;
    Text UnitClass;
    Text UnitDesign;
    Text UnitSkin;
    FVector  UnitLoc;
    int     UnitCount;
    int     UnitDamage;
    int     UnitDead;
    int     UnitHeading;
    
    float    CurrentShipScale = 1.0f;

    // Cached GI (kept)
    USSWGameInstance* SSWInstance = nullptr;

    // Paths
    FString ProjectPath;
    FString FilePath;

    UPROPERTY()
    TMap<FString, FWeaponDesignMeta> WeaponMetaByType;

    // ------------------------------------------------------------
    // Per-ship selection results (reset each time you start a ship)
    // ------------------------------------------------------------
    int32 CurrentShipDecoyWeaponIndex = INDEX_NONE;
    int32 CurrentShipProbeWeaponIndex = INDEX_NONE;
    FString CurrentShipDecoyWeaponType;
    FString CurrentShipProbeWeaponType;
    FString CurrentShipSourceFile;

    // NOTE: these should be constructed somewhere (Initialize or on-demand)
    UPROPERTY(Transient)
    TObjectPtr<UDataTable> DT_Ranks = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UDataTable> DT_Medals = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UDataTable> DT_AwardsAll = nullptr;

    void EnsureAwardTables();

    UPROPERTY()
    TArray<FS_Campaign> AllCampaigns;

    UPROPERTY()
    TMap<FName, FS_Campaign> CampaignMap;
    
    UPROPERTY()
    TMap<int32, int32> CampaignIndexLookup;
};

static FSkinMtlCell ParseSkinMtlCell(TermStruct* Val, const char* Fn);

static uint8 ToByteClamp(double v)
{
    // Legacy files sometimes store 0..255, sometimes 0..1.
    // Heuristic: if <= 1.0, treat as normalized.
    if (v <= 1.0)
    {
        v = v * 255.0;
    }
    v = FMath::Clamp(v, 0.0, 255.0);
    return (uint8)FMath::RoundToInt(v);
}
