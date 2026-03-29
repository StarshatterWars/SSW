/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Campaign.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Campaign defines a strategic military scenario. This class
    owns (or generates) the Mission list that defines the action
    in the campaign.
*/

#pragma once

#include "Types.h"
#include "Text.h"
#include "Term.h"
#include "List.h"
#include "Bitmap.h"
#include "MissionInfo.h"
#include "GameStructs.h"

// Minimal Unreal includes (per port rules):
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // FMath

// +--------------------------------------------------------------------+

class Bitmap;
class CampaignPlan;
class Combatant;
class CombatAction;
class CombatEvent;
class CombatGroup;
class CombatUnit;
class CombatZone;
class DataLoader;
class Mission;
class MissionInfo;
class MissionTemplate;
class StarSystem;
class TermStruct;

// +--------------------------------------------------------------------+

class TemplateList
{
public:
    static const char* TYPENAME() { return "TemplateList"; }

    TemplateList();
    ~TemplateList();

    int               mission_type;
    int               group_type;
    int               index;
    List<MissionInfo> missions;
};

// +--------------------------------------------------------------------+

class Campaign
{
public:
    static const char* TYPENAME() { return "Campaign"; }

    enum CONSTANTS {
        TRAINING_CAMPAIGN = 1,
        DYNAMIC_CAMPAIGN,
        MOD_CAMPAIGN = 100,
        SINGLE_MISSIONS = 1000,
        MULTIPLAYER_MISSIONS,
        CUSTOM_MISSIONS,

        NUM_IMAGES = 6
    };

    Campaign(int id, const char* name = 0);
    Campaign(int id, const char* name, const char* path);

    // NEW: skip legacy filesystem auto-load
    Campaign(int id, const char* name, bool bSkipLoad);

    virtual ~Campaign();

    int operator == (const Campaign& s) const { return name == s.name; }
    int operator <  (const Campaign& s) const { return campaign_id < s.campaign_id; }

    // operations:
    virtual void         Load();
    virtual void         Prep();
    virtual void         Start();
    virtual void         ExecFrame();
    virtual void         Unload();

    virtual void         Clear();
    virtual void         CommitExpiredActions();
    virtual void         LockoutEvents(int seconds);
    virtual void         CheckPlayerGroup();
    CombatGroup*         FindFirstPlayableGroup(CombatGroup* Group);
    void                 CreatePlanners();

    // NEW: DataTable-backed campaign path
    void                 LoadFromData(const FS_Campaign& Data);
    static Campaign* CreateFromData(const FS_Campaign& Data);
    static Campaign* SelectFromData(const FS_Campaign& Data);

    void DumpMissionList(const FString& Label, const List<MissionInfo>& SourceList) const;
    void DumpTemplateBuckets(const FString& Label, const List<TemplateList>& SourceList) const;
    void DumpAllMissionState(const FString& Label) const;

    const FS_CampaignMission* FindCampaignMissionData(int32 MissionType, CombatGroup* Squadron) const;

    const FS_CampaignMission* FindCampaignMissionByScript(const char* ScriptName) const;

    const List<TemplateList>& GetTemplates() const { return templates; }

    // accessors:
    const char* Name()         const { return name; }
    const char* Description()  const { return description; }
    const char* Path()         const { return path; }

    const char* Situation()    const { return situation; }
    const char* Orders()       const { return orders; }

    void                 SetSituation(const char* s) { situation = s; }
    void                 SetOrders(const char* o) { orders = o; }

    int                  GetPlayerTeamScore();
    List<MissionInfo>& GetMissionList() { return missions; }
    List<Combatant>& GetCombatants() { return combatants; }
    List<CombatZone>& GetZones() { return zones; }
    List<StarSystem>& GetSystemList() { return systems; }
    List<CombatAction>& GetActions() { return actions; }
    List<CombatEvent>& GetEvents() { return events; }
    CombatEvent* GetLastEvent();

    CombatAction* FindAction(int id);

    int                  CountNewEvents() const;

    int                  GetPlayerIFF();
    CombatGroup*         GetPlayerGroup() { return player_group; }
    void                 SetPlayerGroup(CombatGroup* pg);
    CombatUnit*          GetPlayerUnit() { return player_unit; }
    void                 SetPlayerUnit(CombatUnit* pu);

    Combatant* GetCombatant(const char* name);
    CombatGroup* FindGroup(int iff, int type, int id);
    CombatGroup* FindGroup(int iff, int type, CombatGroup* near_group = 0);
    CombatGroup* FindStrikeTarget(int iff, CombatGroup* strike_group);

    StarSystem* GetSystem(const char* sys);
    CombatZone* GetZone(const char* rgn);
    MissionInfo* CreateNewMission();
    void                 DeleteMission(int id);
    Mission* GetMission();
    Mission* GetMission(int id);
    Mission* GetMissionByFile(const char* filename);
    MissionInfo* GetMissionInfo(int id);
    MissionInfo* FindMissionTemplate(int msn_type, CombatGroup* player_group);
    const FS_CampaignMission* FindCampaignMissionById(int32 id) const;
    void                 ReloadMission(int id);
    void                 LoadNetMission(int id, const char* net_mission);
    void                 StartMission();
    void                 RollbackMission();

    void                 SetCampaignId(int id);
    int                  GetCampaignId()   const { return campaign_id; }
    void                 SetMissionId(int id);
    int                  GetMissionId()    const { return mission_id; }
    Bitmap*              GetImage(int n) { return &image[n]; }

    double               GetTime()         const { return time;      }
    double               GetStartTime()    const { return startTime; }

    void                 SetStartTime(double t)  { startTime = t;    }
    void                 SetTime(double t)       { time = t;         }

    double               GetLoadTime()     const { return loadTime;  } 
    void                 SetLoadTime(double t) { loadTime = t;       }

    double               GetUpdateTime()   const { return updateTime; }
    void                 SetUpdateTime(double t) { updateTime = t;    }

	void                 SetScripted(bool s) { scripted = s; }
    bool                 InCutscene()      const;
    bool                 IsDynamic()       const;
    bool                 IsTraining()      const;
    bool                 IsScripted()      const;
    bool                 IsSequential()    const;
    bool                 IsSaveGame()      const { return loaded_from_savegame; }
    void                 SetSaveGame(bool s) { loaded_from_savegame = s; }

    bool                 IsActive()        const { return campaign_status == ECampaignStatus::ACTIVE; }
    bool                 IsComplete()      const { return campaign_status == ECampaignStatus::SUCCESS; }
    bool                 IsFailed()        const { return campaign_status == ECampaignStatus::FAILED; }
    void                 SetCampaignStatus(ECampaignStatus s);
    ECampaignStatus      GetCampaignStatus()       const { return campaign_status; }

    int                  GetAllCombatUnits(int iff, List<CombatUnit>& units);

    static void          Initialize();
    static void          Close();
    static Campaign*     GetCampaign();
    static List<Campaign>& GetAllCampaigns();
    static int           GetLastCampaignId();
    static Campaign*     SelectCampaign(const char* name);
    static Campaign*     CreateCustomCampaign(const char* name, const char* path);

    static double        GetStardate();

protected:
    void                 LoadCampaign(DataLoader* loader, bool full = false);
    void                 LoadTemplateList(DataLoader* loader);
    void                 LoadMissionList(DataLoader* loader);
    void                 LoadCustomMissions(DataLoader* loader);

    void                 ParseGroup(
        TermStruct* val,
        CombatGroup* force,
        CombatGroup* clone,
        const char* filename);

    void                 ParseAction(
        TermStruct* val,
        const char* filename);

    CombatGroup* CloneOver(
        CombatGroup* force,
        CombatGroup* clone,
        CombatGroup* group);

    void                 SelectDefaultPlayerGroup(CombatGroup* g, int type);
    TemplateList* GetTemplateList(int msn_type, int grp_type);
    void SetCampaignData(const FS_Campaign* InData);

    // attributes:
    int                  campaign_id;
    ECampaignStatus      campaign_status;
    char                 filename[64];
    char                 path[64];
    Text                 name;
    Text                 description;
    Text                 situation;
    Text                 orders;
    Bitmap               image[NUM_IMAGES];

    bool                 scripted;
    bool                 sequential;
    bool                 loaded_from_savegame;

    List<Combatant>      combatants;
    List<StarSystem>     systems;
    List<CombatZone>     zones;
    List<CampaignPlan>   planners;
    List<MissionInfo>    missions;
    List<TemplateList>   templates;
    List<CombatAction>   actions;
    List<CombatEvent>    events;
    CombatGroup* player_group;
    CombatUnit* player_unit;

    int                  mission_id;
    Mission* mission;
    Mission* net_mission;

    double               time;
    double               loadTime;
    double               startTime;
    double               updateTime;
    int                  lockout;

    double                CampaignStartStardate = 0.0;

    // NEW: optional traceability for DT-backed campaigns
    FName                SourceRowName = NAME_None;

protected:
        const FS_Campaign* CampaignData = nullptr;

protected:
    void LoadCombatantsFromData(const FS_Campaign& Data);
    void LoadActionsFromData(const FS_Campaign& Data);

    void ApplyCombatantGroupsFromData(
        const TArray<FS_CombatantGroup>& GroupRows,
        CombatGroup* SourceForce,
        CombatGroup* CloneForce
    );

    void AddActionRequirementFromData(
        CombatAction* Action,
        const FS_CampaignReq& ReqRow
    );

    FString GetCombatantNameString(EEMPIRE_NAME Empire) const;
};