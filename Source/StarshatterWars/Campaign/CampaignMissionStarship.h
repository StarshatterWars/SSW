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

#include "CoreMinimal.h"
#include "Math/Vector.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"

class Campaign;
class CampaignMissionRequest;
class CombatGroup;
class CombatUnit;
class CombatZone;
class Mission;
class MissionElement;
class MissionInfo;
class MissionTemplate;

class CampaignMissionStarship
{
public:
    static const char* TYPENAME() { return "CampaignMissionStarship"; }

    CampaignMissionStarship(Campaign* c);
    virtual ~CampaignMissionStarship();

    virtual void   CreateMission(CampaignMissionRequest* request);

protected:
    virtual Mission* GenerateMission(int id);
    virtual void     SelectType();
    virtual void     SelectRegion();

    virtual void     GenerateStandardElements();
    virtual void     ProcessGroupRecursive(CombatGroup* g, const FString& MissionRegion);

    virtual void     GenerateMissionElements();
    virtual void     CreateElements(CombatGroup* g);
    virtual void     CreateSquadron(CombatGroup* g);
    virtual void     CreatePlayer();

    virtual void     CreateWards();
    virtual void     CreateWardFreight();

    virtual void     CreateEscorts();

    virtual void     CreateTargets();
    virtual void     CreateTargetsAssault();
    virtual void     CreateTargetsPatrol();
    virtual void     CreateTargetsCarrier();
    virtual void     CreateTargetsFreightEscort();
    virtual int      CreateRandomTarget(const char* rgn, FVector base_loc);

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

    CombatUnit* player_unit = nullptr;
    CombatGroup* player_group = nullptr;
    CombatGroup* strike_group = nullptr;
    CombatGroup* strike_target = nullptr;
    Mission* mission = nullptr;

    List<MissionElement>    player_group_elements;

    MissionElement* player = nullptr;
    MissionElement* ward = nullptr;
    MissionElement* prime_target = nullptr;
    MissionElement* escort = nullptr;

    int                     ownside = 0;
    int                     enemy = -1;
    int                     mission_type = 0;

    FString                 Existing;

    TSet<CombatGroup*>      ProcessedGroups;
    TSet<FString>           ProcessedGroupKeys;
};