/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanAssignment.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanAssignment creates combat assignments for
    assets within each combat zone as the third step in
    force tasking.
*/

#pragma once

#include "Types.h"
#include "CampaignPlan.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class CombatGroup;
class CombatUnit;
class CombatZone;

// +--------------------------------------------------------------------+

class CampaignPlanAssignment : public CampaignPlan
{
public:
    static const char* TYPENAME() { return "CampaignPlanAssignment"; }

    CampaignPlanAssignment(Campaign* c) : CampaignPlan(c) {}
    virtual ~CampaignPlanAssignment() {}

    // operations:
    virtual void   ExecFrame() override;

protected:
    virtual void   ProcessCombatant(Combatant* c);
    virtual void   ProcessZone(Combatant* c, CombatZone* zone);
    virtual void   BuildZoneList(CombatGroup* g, CombatZone* zone, List<CombatGroup>& list);
    virtual void   BuildAssetList(const int* pref, List<CombatGroup>& avail, List<CombatGroup>& assets);
};

