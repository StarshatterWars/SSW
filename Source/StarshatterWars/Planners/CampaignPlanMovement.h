/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanMovement.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanMovement simulates random patrol movements
    of starship groups between missions. This agitation
    keeps the ships from bunching up in the middle of a
    sector.
*/

#pragma once

#include "Types.h"
#include "CampaignPlan.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class CombatUnit;

// +--------------------------------------------------------------------+

class CampaignPlanMovement : public CampaignPlan
{
public:
    static const char* TYPENAME() { return "CampaignPlanMovement"; }

    CampaignPlanMovement(Campaign* c) : CampaignPlan(c) {}
    virtual ~CampaignPlanMovement() {}

    // operations:
    virtual void      ExecFrame();

protected:
    void              MoveUnit(CombatUnit* u);

    List<CombatUnit>  all_units;
};
