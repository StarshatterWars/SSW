/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignMissionStarship.h
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    CampaignMissionStarship generates missions and mission
    info for the player's STARSHIP GROUP as part of a
    dynamic campaign.
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"

// Unreal minimal includes:
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // Math

// +--------------------------------------------------------------------+

class Campaign;
class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;
class MissionInfo;
class MissionTemplate;

// +--------------------------------------------------------------------+

class CampaignMissionStarship
{
public:
    static const char* TYPENAME() { return "CampaignMissionStarship"; }

    CampaignMissionStarship(Campaign* c);
    virtual ~CampaignMissionStarship();

    virtual void   CreateMission(CampaignMissionRequest* request);

protected:
    virtual Mission* GenerateMission(int id);
    virtual void      SelectType();
    virtual void      SelectRegion();
    virtual void      GenerateStandardElements();
    virtual void      GenerateMissionElements();
    virtual void      CreateElements(CombatGroup* g);
    virtual void      CreateSquadron(CombatGroup* g);
    virtual void      CreatePlayer();

    virtual void      CreateWards();
    virtual void      CreateWardFreight();

    virtual void      CreateEscorts();

    virtual void      CreateTargets();
    virtual void      CreateTargetsAssault();
    virtual void      CreateTargetsPatrol();
    virtual void      CreateTargetsCarrier();
    virtual void      CreateTargetsFreightEscort();
    virtual int       CreateRandomTarget(const char* rgn, FVector base_loc);

    virtual MissionElement* CreateSingleElement(CombatGroup* g, CombatUnit* u);

    virtual MissionElement* CreateFighterPackage(CombatGroup* squadron,
        int         count,
        int         role);

    virtual CombatGroup* FindSquadron(int iff, int type);
    virtual CombatUnit* FindCarrier(CombatGroup* g);

    virtual void         DefineMissionObjectives();
    virtual MissionInfo* DescribeMission();
    virtual void         Exit();

    Campaign* campaign;
    CampaignMissionRequest* request;
    MissionInfo* mission_info;

    CombatUnit* player_unit;
    CombatGroup* player_group;
    CombatGroup* strike_group;
    CombatGroup* strike_target;
    Mission* mission;

    List<MissionElement>    player_group_elements;

    MissionElement* player;
    MissionElement* ward;
    MissionElement* prime_target;
    MissionElement* escort;

    int                     ownside;
    int                     enemy;
    int                     mission_type;
};
