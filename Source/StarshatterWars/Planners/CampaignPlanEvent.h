/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanEvent.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanEvent generates simulated combat
    events based on a statistical analysis of the
    combatants within the context of a dynamic
    campaign.
*/

#pragma once

#include "Types.h"
#include "CampaignPlan.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class CombatAction;
class CombatAssignment;
class CombatEvent;
class CombatGroup;
class CombatUnit;
class CombatZone;

// +--------------------------------------------------------------------+

class CampaignPlanEvent : public CampaignPlan
{
public:
    static const char* TYPENAME() { return "CampaignPlanEvent"; }

    CampaignPlanEvent(Campaign* c);
    virtual ~CampaignPlanEvent();

    // operations:
    virtual void   ExecFrame() override;
    virtual void   SetLockout(int seconds);

    virtual bool   ExecScriptedEvents();
    virtual bool   ExecStatisticalEvents();

protected:
    virtual void   ProsecuteKills(CombatAction* action);

    virtual CombatAssignment* ChooseAssignment(CombatGroup* c);
    virtual bool              CreateEvent(CombatAssignment* a);

    virtual CombatEvent* CreateEventDefend(CombatAssignment* a);
    virtual CombatEvent* CreateEventFighterAssault(CombatAssignment* a);
    virtual CombatEvent* CreateEventFighterStrike(CombatAssignment* a);
    virtual CombatEvent* CreateEventFighterSweep(CombatAssignment* a);
    virtual CombatEvent* CreateEventStarship(CombatAssignment* a);

    virtual bool   IsFriendlyAssignment(CombatAssignment* a);
    virtual bool   Success(CombatAssignment* a);
    virtual Text   GetTeamName(CombatGroup* g);

    // attributes:
    int event_time;
};
