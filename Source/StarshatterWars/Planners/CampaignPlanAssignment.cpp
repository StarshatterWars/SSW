/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CampaignPlanAssignment.cpp
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

#include "CampaignPlanAssignment.h"

#include "GameStructs.h"

#include "Campaign.h"
#include "Combatant.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Mission.h"

#include "Logging/LogMacros.h"

static inline void LogWarningIfNoCampaign()
{
	// intentionally empty placeholder for future shared diagnostics
}

// +--------------------------------------------------------------------+

void
CampaignPlanAssignment::ExecFrame()
{
	if (campaign && campaign->IsActive()) {
		// once every few minutes is plenty:
		if (Campaign::GetStardate() - exec_time < 300)
			return;

		ListIter<Combatant> iter = campaign->GetCombatants();
		while (++iter) {
			ProcessCombatant(iter.value());
		}

		exec_time = Campaign::GetStardate();
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanAssignment::ProcessCombatant(Combatant* c)
{
	if (!c || !campaign)
		return;

	CombatGroup* force = c->GetForce();
	if (force) {
		force->CalcValue();
		force->ClearAssignments();
	}

	ListIter<CombatZone> zone = campaign->GetZones();
	while (++zone) {
		ProcessZone(c, zone.value());
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanAssignment::BuildZoneList(CombatGroup* g, CombatZone* zone, List<CombatGroup>& groups)
{
	if (!g)
		return;

	if (g->GetAssignedZone() == zone)
		groups.append(g);

	ListIter<CombatGroup> iter = g->GetComponents();
	while (++iter)
		BuildZoneList(iter.value(), zone, groups);
}

// +--------------------------------------------------------------------+

void
CampaignPlanAssignment::BuildAssetList(const int* pref, List<CombatGroup>& groups, List<CombatGroup>& assets)
{
	if (!pref)
		return;

	while (*pref) {
		ListIter<CombatGroup> g = groups;
		while (++g) {
			if ((int)g->GetType() == *pref && g->CountUnits() > 0)
				assets.append(g.value());
		}

		pref++;
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanAssignment::ProcessZone(Combatant* c, CombatZone* zone)
{
	if (!c || !zone)
		return;

	List<CombatGroup> groups;
	BuildZoneList(c->GetForce(), zone, groups);

	ZoneForce* force = zone->FindForce(c->GetIFF());
	if (!force)
		return;

	// defensive assignments:
	ListIter<CombatGroup> def = force->GetDefendList();
	while (++def) {
		List<CombatGroup> assets;
		BuildAssetList(CombatGroup::PreferredDefender(def->GetType()), groups, assets);

		ListIter<CombatGroup> g = assets;
		while (++g) {
			CombatAssignment* a =
				new CombatAssignment((int)EMissionType::DEFEND,
					def.value(),
					g.value());

			if (a)
				g->GetAssignments().append(a);
		}
	}

	// offensive assignments:
	ListIter<CombatGroup> tgt = force->GetTargetList();
	while (++tgt) {
		CombatGroup* target = tgt.value();
		if (!target)
			continue;

		List<CombatGroup> assets;
		BuildAssetList(CombatGroup::PreferredAttacker(tgt->GetType()), groups, assets);

		ListIter<CombatGroup> g = assets;
		while (++g) {
			CombatGroup* asset = g.value();
			if (!asset)
				continue;

			int mtype = (int)EMissionType::ASSAULT;

			if (target->IsStrikeTarget())
				mtype = (int)EMissionType::STRIKE;

			else if (target->IsFighterGroup())
				mtype = (int)EMissionType::SWEEP;

			else if (target->GetType() == ECOMBATGROUP_TYPE::LCA_SQUADRON)
				mtype = (int)EMissionType::INTERCEPT;

			CombatAssignment* a =
				new CombatAssignment(mtype, target, asset);

			if (a)
				g->GetAssignments().append(a);
		}
	}
}
