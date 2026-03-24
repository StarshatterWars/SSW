/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignMissionFighter.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    CampaignMissionFighter generates missions and mission
    info for the player's FIGHTER SQUADRON as part of a
    dynamic campaign.

    UE PORT NOTES
    =============
    - Keeps original class/member naming and overall structure.
    - Uses UE-compatible includes and logging.
    - Keeps legacy Text fields instead of forcing FString.
    - Does NOT yet reintroduce the full original mission-generation logic.
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "List.h"
#include "Text.h"

// Unreal:
#include "CoreMinimal.h"

class Campaign;
class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;
class MissionInfo;
class MissionTemplate;

class CampaignMissionFighter
{
public:
    static const char* TYPENAME() { return "CampaignMissionFighter"; }

    explicit CampaignMissionFighter(Campaign* c);
    virtual ~CampaignMissionFighter();

    virtual void CreateMission(CampaignMissionRequest* request);

protected:
    virtual Mission* GenerateMission(int id);
    virtual void     SelectType();
    virtual void     SelectRegion();
    virtual void     GenerateStandardElements();
    virtual void     GenerateMissionElements();
    virtual void     CreateElements(CombatGroup* g);
    virtual void     CreateSquadron(CombatGroup* g);
    virtual void     CreatePlayer(CombatGroup* g);

    virtual void     CreatePatrols();
    virtual void     CreateWards();
    virtual void     CreateWardFreight();
    virtual void     CreateWardShuttle();
    virtual void     CreateWardStrike();

    virtual void     CreateEscorts();

    virtual void     CreateTargets();
    virtual void     CreateTargetsPatrol();
    virtual void     CreateTargetsSweep();
    virtual void     CreateTargetsIntercept();
    virtual void     CreateTargetsFreightEscort();
    virtual void     CreateTargetsShuttleEscort();
    virtual void     CreateTargetsStrikeEscort();
    virtual void     CreateTargetsStrike();
    virtual void     CreateTargetsAssault();
    virtual int      CreateRandomTarget(const char* rgn, Point base_loc);

    virtual bool     IsGroundObjective(CombatGroup* obj);

    virtual void     PlanetaryInsertion(MissionElement* elem);
    virtual void     OrbitalInsertion(MissionElement* elem);

    virtual MissionElement* CreateSingleElement(CombatGroup* g, CombatUnit* u);
    virtual MissionElement* CreateFighterPackage(CombatGroup* squadron, int count, int role);

    virtual CombatGroup* FindSquadron(int iff, int type);
    virtual CombatUnit* FindCarrier(CombatGroup* g);

    virtual void         DefineMissionObjectives();
    virtual MissionInfo* DescribeMission();
    virtual void         Exit();

protected:
    Campaign* campaign = nullptr;
    CampaignMissionRequest* request = nullptr;
    MissionInfo* mission_info = nullptr;

    CombatGroup* squadron = nullptr;
    CombatGroup* strike_group = nullptr;
    CombatGroup* strike_target = nullptr;
    Mission* mission = nullptr;

    MissionElement* player_elem = nullptr;
    MissionElement* carrier_elem = nullptr;
    MissionElement* ward = nullptr;
    MissionElement* prime_target = nullptr;
    MissionElement* escort = nullptr;

    Text                    air_region;
    Text                    orb_region;

    bool                    airborne = false;
    bool                    airbase = false;

    int                     ownside = 0;
    int                     enemy = -1;
    int                     mission_type = 0;
};