/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanMission.h
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

#pragma once

#include "Types.h"
#include "CampaignPlan.h"

// Unreal minimal math includes (project standard):
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // FMath

// +--------------------------------------------------------------------+

class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;

// +--------------------------------------------------------------------+

class CampaignPlanMission : public CampaignPlan
{
public:
    static const char* TYPENAME() { return "CampaignPlanMission"; }

    CampaignPlanMission(Campaign* c) : CampaignPlan(c), start(0), slot(0) {}
    virtual ~CampaignPlanMission() {}

    // operations:
    virtual void                 ExecFrame();

protected:
    virtual void                 SelectStartTime();
    virtual CampaignMissionRequest* PlanCampaignMission();
    virtual CampaignMissionRequest* PlanStrategicMission();
    virtual CampaignMissionRequest* PlanRandomStarshipMission();
    virtual CampaignMissionRequest* PlanRandomFighterMission();
    virtual CampaignMissionRequest* PlanStarshipMission();
    virtual CampaignMissionRequest* PlanFighterMission();

    CombatGroup* player_group = nullptr;
    int                          start = 0;
    int                          slot = 0;
};
