/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanStrategic.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanStrategic prioritizes targets and defensible
    allied forces as the first step in force tasking.  This
    algorithm computes which enemy resources are most important
    to attack, based on the AI value of each combat group, and
    strategic weighting factors that help shape the strategy
    to the objectives for the current campaign.
*/

#pragma once

#include "Types.h"
#include "CampaignPlan.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class CampaignPlanStrategic : public CampaignPlan
{
public:
    static const char* TYPENAME() { return "CampaignPlanStrategic"; }

    CampaignPlanStrategic(Campaign* c) : CampaignPlan(c) {}
    virtual ~CampaignPlanStrategic() {}

    // operations:
    virtual void   ExecFrame() override;

protected:
    void           PlaceGroup(CombatGroup* g);

    void           ScoreCombatant(Combatant* c);

    void           ScoreDefensible(Combatant* c);
    void           ScoreDefend(Combatant* c, CombatGroup* g);

    void           ScoreTargets(Combatant* c, Combatant* t);
    void           ScoreTarget(Combatant* c, CombatGroup* g);

    void           ScoreNeeds(Combatant* c);

    // zone allocation:
    void           BuildGroupList(CombatGroup* g, List<CombatGroup>& groups);
    void           AssignZones(Combatant* c);
    void           ResolveZoneMovement(CombatGroup* g);
};
