/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CampaignPlanStrategic.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	=========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	CampaignPlanStrategic prioritizes targets and defensible
	allied forces as the first step in force tasking.
*/

#include "CampaignPlanStrategic.h"

#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "CombatZone.h"
#include "Random.h"

#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ExecFrame()
{
	if (campaign && campaign->IsActive()) {
		if (Campaign::Stardate() - exec_time < 300)
			return;

		ListIter<CombatZone> zone = campaign->GetZones();
		while (++zone)
			zone->Clear();

		ListIter<Combatant>  iter = campaign->GetCombatants();
		while (++iter) {
			Combatant* c = iter.value();
			CombatGroup* force = c ? c->GetForce() : nullptr;

			if (!force)
				continue;

			force->CalcValue();

			PlaceGroup(force);
			ScoreCombatant(c);
			ScoreNeeds(c);

			force->ClearUnlockedZones();
			AssignZones(c);
			ResolveZoneMovement(force);
		}

		exec_time = Campaign::Stardate();
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::PlaceGroup(CombatGroup* g)
{
	if (!g)
		return;

	Text        rgn = g->GetRegion();
	CombatZone* zone = campaign ? campaign->GetZone(rgn) : nullptr;

	// if we couldn't find anything suitable,
	// just pick a zone at random:
	if (!zone && campaign && g->IsMovable()) {
		const int nzones = campaign->GetZones().size();
		const int n = (nzones > 0) ? (RandomIndex() % nzones) : 0;

		if (nzones > 0)
			zone = campaign->GetZones().at(n);

		Text assigned_rgn;
		if (zone && !campaign->GetZone(rgn)) {
			assigned_rgn = *zone->GetRegions().at(0);
			g->AssignRegion(assigned_rgn);
		}
	}

	if (zone && !zone->HasGroup(g))
		zone->AddGroup(g);

	ListIter<CombatGroup> iter = g->GetComponents();
	while (++iter)
		PlaceGroup(iter.value());
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ScoreCombatant(Combatant* c)
{
	if (!c || !campaign)
		return;

	// prep lists:
	c->GetDefendList().clear();
	c->GetTargetList().clear();

	ScoreDefensible(c);

	ListIter<Combatant> iter = campaign->GetCombatants();
	while (++iter) {
		if (iter->GetIFF() > 0 && iter->GetIFF() != c->GetIFF())
			ScoreTargets(c, iter.value());
	}

	// sort lists:
	c->GetDefendList().sort();
	c->GetTargetList().sort();
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ScoreDefensible(Combatant* c)
{
	if (c && c->GetForce())
		ScoreDefend(c, c->GetForce());
}

void
CampaignPlanStrategic::ScoreDefend(Combatant* c, CombatGroup* g)
{
	if (!c || !g || g->IsReserve() || !campaign)
		return;

	if (g->IsDefensible()) {
		g->SetPlanValue(g->Value());
		c->GetDefendList().append(g);

		CombatZone* zone = campaign->GetZone(g->GetRegion());
		ZoneForce* force = nullptr;

		if (zone)
			force = zone->FindForce(c->GetIFF());

		if (force)
			force->GetDefendList().append(g);
	}

	ListIter<CombatGroup> iter = g->GetComponents();
	while (++iter) {
		ScoreDefend(c, iter.value());
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ScoreTargets(Combatant* c, Combatant* t)
{
	if (c && t && t->GetForce())
		ScoreTarget(c, t->GetForce());
}

void
CampaignPlanStrategic::ScoreTarget(Combatant* c, CombatGroup* g)
{
	if (!c || !g || !campaign || g->IntelLevel() <= Intel::SECRET)
		return;

	if (g->IsTargetable()) {
		g->SetPlanValue(g->Value() * c->GetTargetStratFactor(g->GetType()));
		c->GetTargetList().append(g);

		CombatZone* zone = campaign->GetZone(g->GetRegion());
		ZoneForce* force = nullptr;

		if (zone)
			force = zone->FindForce(c->GetIFF());

		if (force)
			force->GetTargetList().append(g);
	}

	ListIter<CombatGroup> iter = g->GetComponents();
	while (++iter) {
		ScoreTarget(c, iter.value());
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ScoreNeeds(Combatant* c)
{
	if (!c || !campaign)
		return;

	ListIter<CombatZone> zone = campaign->GetZones();
	while (++zone) {
		ZoneForce* force = zone->FindForce(c->GetIFF());
		if (!force)
			continue;

		// clear needs:
		force->SetNeed(ECOMBATGROUP_TYPE::CARRIER_GROUP, 0);
		force->SetNeed(ECOMBATGROUP_TYPE::BATTLE_GROUP, 0);
		force->SetNeed(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON, 0);
		force->SetNeed(ECOMBATGROUP_TYPE::ATTACK_SQUADRON, 0);
		force->SetNeed(ECOMBATGROUP_TYPE::FIGHTER_SQUADRON, 0);
		force->SetNeed(ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON, 0);

		// what defensive assets are needed in this zone?
		ListIter<CombatGroup> def = force->GetDefendList();
		while (++def) {
			CombatGroup* DefGroup = def.value();
			if (!DefGroup)
				continue;

			const ECOMBATGROUP_TYPE defender_type =
				static_cast<ECOMBATGROUP_TYPE>(*CombatGroup::PreferredDefender(DefGroup->GetType()));
		}

		// what offensive assets are needed in this zone?
		ListIter<CombatGroup> tgt = force->GetTargetList();
		while (++tgt) {
			CombatGroup* TgtGroup = tgt.value();
			if (!TgtGroup)
				continue;

			const ECOMBATGROUP_TYPE attacker_type =
				static_cast<ECOMBATGROUP_TYPE>(*CombatGroup::PreferredAttacker(TgtGroup->GetType()));
		}
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::BuildGroupList(CombatGroup* g, List<CombatGroup>& groups)
{
	if (!g || g->IsReserve())
		return;

	if (g->IsAssignable())
		groups.append(g);

	ListIter<CombatGroup> iter = g->GetComponents();
	while (++iter)
		BuildGroupList(iter.value(), groups);
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::AssignZones(Combatant* c)
{
	if (!c || !campaign || !c->GetForce())
		return;

	// find the list of assignable groups, in priority order:
	List<CombatGroup> groups;
	BuildGroupList(c->GetForce(), groups);
	groups.sort();

	// for each group, assign a zone:
	ListIter<CombatGroup> g_iter = groups;

	// first pass: fighter and attack squadrons assigned to star bases
	while (++g_iter) {
		CombatGroup* g = g_iter.value();
		if (!g)
			continue;

		const int gtype = (int) g->GetType();

		if (gtype == (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON ||
			gtype == (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON ||
			gtype == (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON) {

			CombatGroup* parent = g->GetParent();

			if (parent && parent->GetType() == ECOMBATGROUP_TYPE::WING)
				parent = parent->GetParent();

			if (!parent || parent->GetType() == ECOMBATGROUP_TYPE::CARRIER_GROUP)
				continue;

			// these groups are attached to fixed resources,
			// so they must be assigned to the parent's zone:
			CombatZone* parent_zone = campaign->GetZone(parent->GetRegion());

			if (parent_zone) {
				ZoneForce* parent_force = parent_zone->FindForce(g->GetIFF());

				if (parent_force) {
					g->SetAssignedZone(parent_zone);
					parent_force->AddNeed(g->GetType(), -(g->Value()));
				}
			}
		}
	}

	// second pass: carrier groups
	g_iter.reset();
	while (++g_iter) {
		CombatGroup* g = g_iter.value();
		if (!g)
			continue;

		const int gtype = (int) g->GetType();

		if (gtype == (int) ECOMBATGROUP_TYPE::CARRIER_GROUP) {
			int         current_zone_need = 0;
			int         highest_zone_need = 0;
			CombatZone* highest_zone = nullptr;
			ZoneForce* highest_force = nullptr;
			CombatZone* current_zone = nullptr;
			ZoneForce* current_force = nullptr;

			List<CombatZone> possible_zones;

			if (g->IsZoneLocked()) {
				current_zone = g->GetAssignedZone();
				if (current_zone)
					current_force = current_zone->FindForce(g->GetIFF());
			}
			else {
				ListIter<CombatZone> z_iter = campaign->GetZones();
				while (++z_iter) {
					CombatZone* zone = z_iter.value();
					if (!zone)
						continue;

					ZoneForce* force = zone->FindForce(g->GetIFF());
					if (!force)
						continue;

					int need =
						force->GetNeed(ECOMBATGROUP_TYPE::CARRIER_GROUP) +
						force->GetNeed(ECOMBATGROUP_TYPE::ATTACK_SQUADRON) +
						force->GetNeed(ECOMBATGROUP_TYPE::FIGHTER_SQUADRON) +
						force->GetNeed(ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON);

					if (g->IsSystemLocked() && zone->GetSystem() != g->GetAssignedSystem())
						continue;

					possible_zones.append(zone);

					if (zone->HasRegion(g->GetRegion())) {
						current_zone_need = need;
						current_zone = zone;
						current_force = force;
					}

					if (need > highest_zone_need) {
						highest_zone_need = need;
						highest_zone = zone;
						highest_force = force;
					}
				}
			}

			CombatZone* assigned_zone = current_zone;
			ZoneForce* assigned_force = current_force;

			if (highest_zone_need > current_zone_need) {
				assigned_zone = highest_zone;
				assigned_force = highest_force;
			}

			// if we couldn't find anything suitable,
			// just pick a zone at random:
			if (!assigned_zone) {
				if (possible_zones.isEmpty())
					possible_zones.append(campaign->GetZones());

				const int nzones = possible_zones.size();
				const int n = (nzones > 0) ? (RandomIndex() % nzones) : 0;

				if (nzones > 0) {
					assigned_zone = possible_zones.at(n);
					assigned_force = assigned_zone ? assigned_zone->FindForce(g->GetIFF()) : nullptr;
				}
			}

			if (assigned_force && assigned_zone) {
				Text assigned_rgn;
				if (!campaign->GetZone(g->GetRegion())) {
					assigned_rgn = *assigned_zone->GetRegions().at(0);
					g->AssignRegion(assigned_rgn);
				}

				g->SetAssignedZone(assigned_zone);
				assigned_force->AddNeed(g->GetType(), -(g->Value()));

				// also assign the carrier's wing and squadrons to the same zone:
				ListIter<CombatGroup> squadron = g->GetComponents();
				while (++squadron) {
					squadron->SetAssignedZone(assigned_zone);
					assigned_force->AddNeed(squadron->GetType(), -(squadron->Value()));

					if (squadron->GetType() == ECOMBATGROUP_TYPE::WING) {
						ListIter<CombatGroup> s = squadron->GetComponents();
						while (++s) {
							s->SetAssignedZone(assigned_zone);
							assigned_force->AddNeed(s->GetType(), -(s->Value()));
						}
					}
				}
			}
		}
	}

	// third pass: everything else
	g_iter.reset();
	while (++g_iter) {
		CombatGroup* g = g_iter.value();
		if (!g)
			continue;

		const int gtype = (int) g->GetType();

		if (gtype == (int) ECOMBATGROUP_TYPE::BATTLE_GROUP || gtype == (int) ECOMBATGROUP_TYPE::DESTROYER_SQUADRON) {
			int         current_zone_need = 0;
			int         highest_zone_need = 0;
			CombatZone* highest_zone = nullptr;
			ZoneForce* highest_force = nullptr;
			CombatZone* current_zone = nullptr;
			ZoneForce* current_force = nullptr;

			List<CombatZone> possible_zones;

			if (g->IsZoneLocked()) {
				current_zone = g->GetAssignedZone();
				if (current_zone)
					current_force = current_zone->FindForce(g->GetIFF());
			}
			else {
				ListIter<CombatZone> z_iter = campaign->GetZones();
				while (++z_iter) {
					CombatZone* zone = z_iter.value();
					if (!zone)
						continue;

					ZoneForce* force = zone->FindForce(g->GetIFF());
					if (!force)
						continue;

					int need = force->GetNeed(g->GetType());

					if (g->IsSystemLocked() && zone->GetSystem() != g->GetAssignedSystem())
						continue;

					possible_zones.append(zone);

					// battle groups can do double-duty:
					if (gtype == (int)ECOMBATGROUP_TYPE::BATTLE_GROUP)
						need += force->GetNeed(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON);

					if (zone->HasRegion(g->GetRegion())) {
						current_zone_need = need;
						current_zone = zone;
						current_force = force;
					}

					if (need > highest_zone_need) {
						highest_zone_need = need;
						highest_zone = zone;
						highest_force = force;
					}
				}
			}

			if (highest_zone_need > current_zone_need) {
				g->SetAssignedZone(highest_zone);

				if (highest_force)
					highest_force->AddNeed(g->GetType(), -(g->Value()));
			}
			else {
				if (!current_zone) {
					if (possible_zones.isEmpty())
						possible_zones.append(campaign->GetZones());

					const int nzones = possible_zones.size();
					const int n = (nzones > 0) ? (RandomIndex() % nzones) : 0;

					if (nzones > 0) {
						current_zone = possible_zones.at(n);
						current_force = current_zone ? current_zone->FindForce(g->GetIFF()) : nullptr;
					}
				}

				g->SetAssignedZone(current_zone);

				if (current_force)
					current_force->AddNeed(g->GetType(), -(g->Value()));

				Text assigned_rgn;
				if (!campaign->GetZone(g->GetRegion()) && current_zone) {
					assigned_rgn = *current_zone->GetRegions().at(0);
					g->AssignRegion(assigned_rgn);
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
CampaignPlanStrategic::ResolveZoneMovement(CombatGroup* g)
{
	if (!g || !campaign)
		return;

	CombatZone* zone = g->GetAssignedZone();
	bool        move = false;

	if (zone && !zone->HasRegion(g->GetRegion())) {
		move = true;

		CombatZone* old_zone = g->GetCurrentZone();
		if (old_zone)
			old_zone->RemoveGroup(g);

		zone->AddGroup(g);
	}

	ListIter<CombatGroup> comp = g->GetComponents();
	while (++comp)
		ResolveZoneMovement(comp.value());

	// assign region last, to allow components to
	// resolve their zones:
	if (zone && move)
		g->AssignRegion(*zone->GetRegions().at(0));
}
