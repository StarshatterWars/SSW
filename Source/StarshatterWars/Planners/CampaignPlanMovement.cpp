/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignPlanMovement.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    CampaignPlanMovement simulates random patrol movements
    of starship groups between missions.
*/

#include "CampaignPlanMovement.h"
#include "GameStructs.h"

#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "ShipDesign.h"

#include "Math/UnrealMathUtility.h"   // FMath
#include "Math/Vector.h"              // FVector

// NOTE:
// - This file assumes CombatUnit::Location(), CombatUnit::MoveTo(), CombatGroup::MoveTo()
//   have been updated to use FVector (per project-wide Vec3/Point -> FVector conversion).
// - Starshatter RNG calls are preserved (Random / RandomDirection) and expected to return
//   scalar/direction compatible with FVector usage.

// +--------------------------------------------------------------------+

void
CampaignPlanMovement::ExecFrame()
{
    if (campaign && campaign->IsActive()) {
        if (Campaign::Stardate() - exec_time < 7200)
            return;

        campaign->GetAllCombatUnits(-1, all_units);

        ListIter<CombatUnit> iter = all_units;
        while (++iter) {
            CombatUnit* u = iter.value();

            if (u->IsStarship() && !u->IsStatic())
                MoveUnit(u);
        }

        all_units.clear();

        exec_time = Campaign::Stardate();
    }
}

// +--------------------------------------------------------------------+

void
CampaignPlanMovement::MoveUnit(CombatUnit* u)
{
    if (!u)
        return;

    // starship repair:
    double damage = u->GetSustainedDamage();

    if (damage > 0 && u->GetDesign()) {
        int percent = (int)(100 * damage / u->GetDesign()->integrity);

        if (percent > 50) {
            u->SetSustainedDamage(0.90 * damage);
        }
    }

    FVector loc = u->Location();

    // dir = normalized vector from origin to loc, dist = magnitude:
    FVector dir = loc;
    const double dist = (double)dir.Size();

    if (dist > 0.0)
        dir /= (float)dist; // safe normalize

    const double MAX_RAD = 320e3;
    const double MIN_DIST = 150e3;

    auto ClampZBand = [](FVector& v)
        {
            if (FMath::Abs(v.Z) > 20000.0f)
                v.Z *= 0.1f;
        };

    if (dist < MAX_RAD) {
        const double scale = 1.0 - dist / MAX_RAD;

        // loc += dir * (Random(30e3, 90e3) * scale) + RandomDirection() * 10e3;
        loc += dir * FMath::FRandRange(30000.0f, 90000.0f) * static_cast<float>(scale)
            + FMath::VRand() * 10000.0f;

        ClampZBand(loc);

        u->MoveTo(loc);

        CombatGroup* g = u->GetCombatGroup();
        if (g && g->GetType() > ECOMBATGROUP_TYPE::FLEET && g->GetFirstUnit() == u) {
            g->MoveTo(loc);

            if (g->IntelLevel() > Intel::KNOWN)
                g->SetIntelLevel(Intel::KNOWN);
        }
    }

    else if (dist > 1.25 * MAX_RAD) {
        const double scale = 1.0 - dist / MAX_RAD;

        // loc += dir * (Random(80e3, 120e3) * scale) + RandomDirection() * 3e3;
        loc += dir * FMath::FRandRange(80000.0f, 120000.0f) * static_cast<float>(scale)
            + FMath::VRand() * 3000.0f;

        ClampZBand(loc);

        u->MoveTo(loc);

        CombatGroup* g = u->GetCombatGroup();
        if (g && g->GetType() > ECOMBATGROUP_TYPE::FLEET && g->GetFirstUnit() == u) {
            g->MoveTo(loc);

            if (g->IntelLevel() > Intel::KNOWN)
                g->SetIntelLevel(Intel::KNOWN);
        }
    }

    else {
        // loc += RandomDirection() * 30e3;
        loc += FMath::VRand() * 30000.0f;

        ClampZBand(loc);

        u->MoveTo(loc);

        CombatGroup* g = u->GetCombatGroup();
        if (g && g->GetType() > ECOMBATGROUP_TYPE::FLEET && g->GetFirstUnit() == u) {
            g->MoveTo(loc);

            if (g->IntelLevel() > Intel::KNOWN)
                g->SetIntelLevel(Intel::KNOWN);
        }
    }

    CombatUnit* closest_unit = nullptr;
    double      closest_dist = 1e6;

    ListIter<CombatUnit> iter = all_units;
    while (++iter) {
        CombatUnit* unit = iter.value();

        if (unit->GetCombatGroup() != u->GetCombatGroup() &&
            unit->GetRegion() == u->GetRegion() &&
            !unit->IsDropship()) {

            FVector delta = loc - unit->Location();
            const double d = (double)delta.Size();

            if (d < closest_dist) {
                closest_unit = unit;
                closest_dist = d;
            }
        }
    }

    if (closest_unit && closest_dist < MIN_DIST) {
        FVector delta = loc - closest_unit->Location();
        const double d = (double)delta.Size();

        if (d > 0.0)
            delta /= (float)d; // normalized direction away from closest_unit

        loc += delta * (float)(1.1 * (MIN_DIST - closest_dist));

        ClampZBand(loc);

        u->MoveTo(loc);

        CombatGroup* g = u->GetCombatGroup();
        if (g && g->GetType() > ECOMBATGROUP_TYPE::FLEET && g->GetFirstUnit() == u) {
            g->MoveTo(loc);

            if (g->IntelLevel() > Intel::KNOWN)
                g->SetIntelLevel(Intel::KNOWN);
        }
    }
}
