/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanMission.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanMission generates missions and mission
    info for the player's combat group as part of a
    dynamic campaign.
*/

#include "CampaignPlanMission.h"

// Project shared structs / logging category (per your standard):
#include "GameStructs.h"

#include "CampaignMissionRequest.h"
#include "CampaignMissionFighter.h"
#include "CampaignMissionStarship.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatAction.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Mission.h"
#include "StarSystem.h"

// Unreal minimal math:
#include "Math/UnrealMathUtility.h" // FMath

// +--------------------------------------------------------------------+

void
CampaignPlanMission::ExecFrame()
{
    if (campaign && campaign->IsActive()) {
        player_group = campaign->GetPlayerGroup();
        if (!player_group) return;

        int missionCount = campaign->GetMissionList().size();

        if (missionCount > 0) {
            // starships only get one mission to pick from:
            if (player_group->IsStarshipGroup())
                return;

            // fighters get a maximum of five missions:
            if (missionCount >= 5)
                return;

            // otherwise, once every few seconds is plenty:
            if (Campaign::GetStardate() - exec_time < 1)
                return;
        }

        SelectStartTime();

        if (player_group->IsFighterGroup()) {
            slot++;
            if (slot > 2) slot = 0;

            CampaignMissionRequest* request = PlanFighterMission();
            CampaignMissionFighter  generator(campaign);
            generator.CreateMission(request);
            delete request;
        }

        else if (player_group->IsStarshipGroup()) {
            // starships should always check for campaign and strategic missions
            slot = 0;

            CampaignMissionRequest* request = PlanStarshipMission();
            CampaignMissionStarship generator(campaign);
            generator.CreateMission(request);
            delete request;
        }

        exec_time = Campaign::GetStardate();
    }
}

// +--------------------------------------------------------------------+

void
CampaignPlanMission::SelectStartTime()
{
    const int   HOUR = 3600;  // 60 minutes
    const int   MISSION_DELAY = 1800;  // 30 minutes
    double      base_time = 0;

    List<MissionInfo>& info_list = campaign->GetMissionList();

    if (info_list.size() > 0) {
        MissionInfo* info = info_list[info_list.size() - 1];
        base_time = info->start;
    }

    if (base_time == 0)
        base_time = campaign->GetTime() + MISSION_DELAY;

    start = (int)base_time + MISSION_DELAY;
    start -= start % MISSION_DELAY;
}

// +--------------------------------------------------------------------+

CampaignMissionRequest*
CampaignPlanMission::PlanStarshipMission()
{
    CampaignMissionRequest* request = 0;

    if (!request)  request = PlanCampaignMission();
    if (!request)  request = PlanStrategicMission();
    if (!request)  request = PlanRandomStarshipMission();

    return request;
}

// +--------------------------------------------------------------------+

CampaignMissionRequest*
CampaignPlanMission::PlanFighterMission()
{
    CampaignMissionRequest* request = 0;

    if (!request)  request = PlanCampaignMission();
    if (!request)  request = PlanStrategicMission();
    if (!request)  request = PlanRandomFighterMission();

    return request;
}

// +--------------------------------------------------------------------+

CampaignMissionRequest*
CampaignPlanMission::PlanCampaignMission()
{
    CampaignMissionRequest* request = 0;

    ListIter<CombatAction> iter = campaign->GetActions();
    while (++iter && !request) {
        CombatAction* action = iter.value();

        if (action->Type() != ECombatActionType::MISSION_TEMPLATE)
            continue;

        if (action->IsAvailable()) {

            // only fire each action once every two hours:
            if (action->ExecTime() > 0 && campaign->GetTime() - action->ExecTime() < 7200)
                continue;

            CombatGroup* g = campaign->FindGroup(action->GetIFF(),
                action->AssetType(),
                action->AssetId());

            if (g && (g == player_group ||
                (player_group->GetType() == ECOMBATGROUP_TYPE::WING &&
                    player_group->FindGroup(g->GetType(), g->GetID())))) {

                request = new CampaignMissionRequest(campaign,
                    action->Subtype(),
                    start,
                    g);

                if (request) {
                    request->SetOpposingType(action->OpposingType());
                    request->SetScript(action->GetText());
                }

                action->FireAction();
            }
        }
    }

    return request;
}

// +--------------------------------------------------------------------+

CampaignMissionRequest*
CampaignPlanMission::PlanStrategicMission()
{
    CampaignMissionRequest* request = 0;

    if (slot > 1)
        return request;

    // build list of assignments:
    List<CombatAssignment> assignments;
    assignments.append(player_group->GetAssignments());

    if (player_group->GetType() == ECOMBATGROUP_TYPE::WING) {
        ListIter<CombatGroup> iter = player_group->GetComponents();
        while (++iter) {
            CombatGroup* g = iter.value();
            assignments.append(g->GetAssignments());
        }
    }

    // pick next assignment as basis for mission:
    static int assignment_index = 0;

    if (assignments.size()) {
        if (assignment_index >= assignments.size())
            assignment_index = 0;

        CombatAssignment* a = assignments[assignment_index++];

        request = new CampaignMissionRequest(campaign,
            a->Type(),
            start,
            a->GetResource());

        if (request)
            request->SetObjective(a->GetObjective());
    }

    return request;
}

// +--------------------------------------------------------------------+

static int mission_type_index = -1;
static int mission_types[16] = {
    (int)EMissionType::PATROL,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL,
    (int)EMissionType::PATROL,
    (int)EMissionType::ESCORT_FREIGHT,
    (int)EMissionType::PATROL
};

// +--------------------------------------------------------------------+

CampaignMissionRequest*
CampaignPlanMission::PlanRandomStarshipMission()
{
    int type = (int)EMissionType::PATROL;
    static constexpr int RANDOM_INDEX_MAX = 15;
    int r = FMath::RandRange(0, RANDOM_INDEX_MAX);

    int ownside = player_group->GetIFF();

    if (mission_type_index < 0)
        mission_type_index = r;

    else if (mission_type_index >= 16)
        mission_type_index = 0;

    type = mission_types[mission_type_index++];

    if (type == (int)EMissionType::ESCORT_FREIGHT) {
        CombatGroup* freight = campaign->FindGroup(ownside, (int) ECOMBATGROUP_TYPE::FREIGHT);
        if (!freight || freight->CountUnits() < 1)
            type = (int)EMissionType::PATROL;
    }

    CampaignMissionRequest* request = 0;
    request = new CampaignMissionRequest(campaign, type, start, player_group);

    return request;
}

// +--------------------------------------------------------------------+

static int fighter_mission_index = 0;
static int fighter_mission_types[16] = {
    (int)EMissionType::PATROL,
    (int)EMissionType::SWEEP,
    (int)EMissionType::ESCORT_SHUTTLE,
    (int)EMissionType::AIR_PATROL,
    (int)EMissionType::SWEEP,
    (int)EMissionType::ESCORT_SHUTTLE,
    (int)EMissionType::PATROL,
    (int)EMissionType::PATROL,
    (int)EMissionType::AIR_SWEEP,
    (int)EMissionType::PATROL,
    (int)EMissionType::AIR_PATROL,
    (int)EMissionType::ESCORT_SHUTTLE,
    (int)EMissionType::PATROL,
    (int)EMissionType::SWEEP,
    (int)EMissionType::PATROL,
    (int)EMissionType::AIR_SWEEP
};

CampaignMissionRequest*
CampaignPlanMission::PlanRandomFighterMission()
{
    CampaignMissionRequest* request = 0;
    int                     type = fighter_mission_types[fighter_mission_index++];
    int                     ownside = player_group->GetIFF();
    CombatGroup* primary = player_group;
    CombatGroup* obj = 0;

    if (fighter_mission_index > 15)
        fighter_mission_index = 0;

    if (type == (int)EMissionType::ESCORT_FREIGHT) {
        CombatGroup* freight = campaign->FindGroup(ownside, (int) ECOMBATGROUP_TYPE::FREIGHT);
        if (!freight || freight->CalcValue() < 1)
            type = (int)EMissionType::PATROL;
        else
            obj = freight;
    }

    else if (type == (int)EMissionType::ESCORT_SHUTTLE) {
        CombatGroup* shuttle = campaign->FindGroup(ownside, (int) ECOMBATGROUP_TYPE::LCA_SQUADRON);
        if (!shuttle || shuttle->CalcValue() < 1)
            type = (int)EMissionType::PATROL;
        else
            obj = shuttle;
    }

    else if (primary->GetType() == ECOMBATGROUP_TYPE::WING) {
        if (FMath::RandBool())
            primary = primary->FindGroup(ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);
        else
            primary = primary->FindGroup(ECOMBATGROUP_TYPE::FIGHTER_SQUADRON);
    }

    if (type >= (int)EMissionType::AIR_PATROL && type <= (int)EMissionType::AIR_INTERCEPT) {
        CombatZone* zone = 0;
        bool        airborne = false;

        if (primary)
            zone = primary->GetAssignedZone();

        if (zone && zone->GetRegions().size() > 1) {
            Text        air_region = *zone->GetRegions().at(1);
            StarSystem* system = campaign->GetSystem(zone->GetSystem());

            if (system) {
                OrbitalRegion* rgn = system->FindRegion(air_region);

                if (rgn && rgn->GetType() == Orbital::TERRAIN)
                    airborne = true;
            }
        }

        if (!airborne) {
            if (type == (int)EMissionType::AIR_INTERCEPT)
                type = (int)EMissionType::INTERCEPT;

            else if (type == (int)EMissionType::AIR_SWEEP)
                type = (int)EMissionType::SWEEP;

            else
                type = (int)EMissionType::PATROL;
        }
    }

    request = new CampaignMissionRequest(campaign, type, start, primary);

    if (request)
        request->SetObjective(obj);

    return request;
}

