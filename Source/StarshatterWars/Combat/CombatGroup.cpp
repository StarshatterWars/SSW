/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright (C) 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CombatGroup.cpp
	AUTHOR:       Carlos Bott

	UNREAL PORT:
	- Maintains all variables and methods (names/signatures/members) from CombatGroup.h.
	- Uses UE-compatible shims for Text, List, Geometry types.
	- Replaces CRT/Win32 helpers with Unreal equivalents where appropriate.
*/

#include "CombatGroup.h"

#include "CombatUnit.h"
#include "CombatZone.h"
#include "Combatant.h"
#include "CombatAssignment.h"
#include "Campaign.h"
#include "ShipDesign.h"
#include "Ship.h"

#include "Game.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "GameStructs.h"

// Unreal:
#include "HAL/PlatformMemory.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

// +----------------------------------------------------------------------+
CombatGroup::CombatGroup(ECOMBATGROUP_TYPE t, int n, const char* s, int iff_code, int e, EEMPIRE_NAME InEmpire, CombatGroup* p)
	: type(t)
	, id(n)
	, name(s)
	, iff(iff_code)
	, enemy_intel(e)
	, empire_id(InEmpire)
	, plan_value(0)
	, units()
	, components()
	, live_comp()
	, combatant(0)
	, parent(p)
	, region()
	, location()
	, value(0)
	, unit_index(0)
	, sorties(0)
	, kills(0)
	, points(0)
	, expanded(false)
	, assigned_system()
	, current_zone(0)
	, assigned_zone(0)
	, zone_lock(false)
	, assignments()
	, strategic_direction()
{
	if (parent)
		parent->AddComponent(this);
}

CombatGroup::~CombatGroup()
{
	assignments.destroy();
	components.destroy();
	units.destroy();
}

// +--------------------------------------------------------------------+

void
CombatGroup::AddComponent(CombatGroup* g)
{
	if (g) {
		g->parent = this;
		components.append(g);
	}
}

// +--------------------------------------------------------------------+

bool
CombatGroup::IsAssignable() const
{
	switch (type) {
	case ECOMBATGROUP_TYPE::CARRIER_GROUP:
	case ECOMBATGROUP_TYPE::BATTLE_GROUP:
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
	case ECOMBATGROUP_TYPE::LCA_SQUADRON:
		return CalcValue() > 0;
	}

	return false;
}

bool
CombatGroup::IsTargetable() const
{
	// neutral / non-combatants are notx*strategic* targets
	// for any combatant:
	if (iff < 1 || iff >= 100)
		return false;

	// civilian / non-combatant are notxstrategic targets:
	if (type == ECOMBATGROUP_TYPE::PASSENGER ||
		type == ECOMBATGROUP_TYPE::PRIVATE ||
		type == ECOMBATGROUP_TYPE::MEDICAL ||
		type == ECOMBATGROUP_TYPE::HABITAT)
		return false;

	// must have units of our own to be targetable:
	if (units.size() < 1)
		return false;

	return ((CombatGroup*)this)->CalcValue() > 0;
}

bool
CombatGroup::IsDefensible() const
{
	if (type >= ECOMBATGROUP_TYPE::SUPPORT)
		return CalcValue() > 0;

	return false;
}

bool
CombatGroup::IsStrikeTarget() const
{
	if (type < ECOMBATGROUP_TYPE::BATTALION ||
		type == ECOMBATGROUP_TYPE::MINEFIELD ||   // assault, notxstrike
		type == ECOMBATGROUP_TYPE::PASSENGER ||
		type == ECOMBATGROUP_TYPE::PRIVATE ||
		type == ECOMBATGROUP_TYPE::MEDICAL ||
		type == ECOMBATGROUP_TYPE::HABITAT)
		return false;

	return CalcValue() > 0;
}

// +--------------------------------------------------------------------+

bool
CombatGroup::IsMovable() const
{
	switch (type) {
	case ECOMBATGROUP_TYPE::CARRIER_GROUP:
	case ECOMBATGROUP_TYPE::BATTLE_GROUP:
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
	case ECOMBATGROUP_TYPE::LCA_SQUADRON:
	case ECOMBATGROUP_TYPE::COURIER:
	case ECOMBATGROUP_TYPE::MEDICAL:
	case ECOMBATGROUP_TYPE::SUPPLY:
	case ECOMBATGROUP_TYPE::REPAIR:
	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:
		return true;
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
CombatGroup::IsFighterGroup() const
{
	switch (type) {
	case ECOMBATGROUP_TYPE::WING:
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
		return true;
	}

	return false;
}

bool
CombatGroup::IsStarshipGroup() const
{
	switch (type) {
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
	case ECOMBATGROUP_TYPE::BATTLE_GROUP:
	case ECOMBATGROUP_TYPE::CARRIER_GROUP:
		return true;
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
CombatGroup::IsReserve() const
{
	if (enemy_intel <= Intel::RESERVE)
		return true;

	if (parent)
		return parent->IsReserve();

	return false;
}

// +--------------------------------------------------------------------+

const int* CombatGroup::PreferredAttacker(ECOMBATGROUP_TYPE InType)
{
	static int Pref[8];
	FMemory::Memzero(Pref, sizeof(Pref));

	switch (InType) {
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
		Pref[0] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[2] = (int)ECOMBATGROUP_TYPE::CARRIER_GROUP;
		Pref[3] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::BATTLE_GROUP:
		Pref[0] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[1] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[2] = (int)ECOMBATGROUP_TYPE::CARRIER_GROUP;
		Pref[3] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::CARRIER_GROUP:
		Pref[0] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[2] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[3] = (int)ECOMBATGROUP_TYPE::CARRIER_GROUP;
		break;

	case ECOMBATGROUP_TYPE::LCA_SQUADRON:
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:
		Pref[0] = (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::STATION:
		Pref[0] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[1] = (int)ECOMBATGROUP_TYPE::CARRIER_GROUP;
		break;

	case ECOMBATGROUP_TYPE::STARBASE:
	case ECOMBATGROUP_TYPE::BATTERY:
	case ECOMBATGROUP_TYPE::MISSILE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::MINEFIELD:
	case ECOMBATGROUP_TYPE::COMM_RELAY:
	case ECOMBATGROUP_TYPE::EARLY_WARNING:
	case ECOMBATGROUP_TYPE::FWD_CONTROL_CTR:
	case ECOMBATGROUP_TYPE::ECM:
		Pref[0] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		Pref[2] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::COURIER:
	case ECOMBATGROUP_TYPE::MEDICAL:
	case ECOMBATGROUP_TYPE::SUPPLY:
	case ECOMBATGROUP_TYPE::REPAIR:
		Pref[0] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[2] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::FACTORY:
	case ECOMBATGROUP_TYPE::REFINERY:
	case ECOMBATGROUP_TYPE::RESOURCE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::TRANSPORT:
	case ECOMBATGROUP_TYPE::NETWORK:
	case ECOMBATGROUP_TYPE::HABITAT:
	case ECOMBATGROUP_TYPE::STORAGE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		break;

	default:
		break;
	}

	return Pref;
}

// +--------------------------------------------------------------------+

const int* CombatGroup::PreferredDefender(ECOMBATGROUP_TYPE InType)
{
	static int Pref[8];
	FMemory::Memzero(Pref, sizeof(Pref));

	switch (InType) {
	case ECOMBATGROUP_TYPE::STATION:
		Pref[0] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[1] = (int)ECOMBATGROUP_TYPE::CARRIER_GROUP;
		Pref[2] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::STARBASE:
	case ECOMBATGROUP_TYPE::MINEFIELD:
	case ECOMBATGROUP_TYPE::BATTERY:
	case ECOMBATGROUP_TYPE::MISSILE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::COMM_RELAY:
	case ECOMBATGROUP_TYPE::EARLY_WARNING:
	case ECOMBATGROUP_TYPE::FWD_CONTROL_CTR:
	case ECOMBATGROUP_TYPE::ECM:
		Pref[0] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::COURIER:
	case ECOMBATGROUP_TYPE::MEDICAL:
	case ECOMBATGROUP_TYPE::SUPPLY:
	case ECOMBATGROUP_TYPE::REPAIR:
		Pref[0] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		Pref[2] = (int)ECOMBATGROUP_TYPE::ATTACK_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::FACTORY:
	case ECOMBATGROUP_TYPE::REFINERY:
	case ECOMBATGROUP_TYPE::RESOURCE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::TRANSPORT:
	case ECOMBATGROUP_TYPE::NETWORK:
	case ECOMBATGROUP_TYPE::HABITAT:
	case ECOMBATGROUP_TYPE::STORAGE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::FIGHTER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON;
		break;

	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:
		Pref[0] = (int)ECOMBATGROUP_TYPE::DESTROYER_SQUADRON;
		Pref[1] = (int)ECOMBATGROUP_TYPE::BATTLE_GROUP;
		break;

		// ship groups / squadrons: no preferred defender list here
	default:
		break;
	}

	return Pref;
}

// +--------------------------------------------------------------------+

CombatGroup*
CombatGroup::FindGroup(ECOMBATGROUP_TYPE t, int n)
{
	CombatGroup* result = 0;

	if (type == t && (n < 0 || id == n))
		result = this;

	ListIter<CombatGroup> group = components;
	while (!result && ++group) {
		result = group->FindGroup(t, n);
	}

	return result;
}

// +--------------------------------------------------------------------+

CombatGroup*
CombatGroup::Clone(bool deep)
{
	CombatGroup* clone = new
		CombatGroup(type, id, name, iff, enemy_intel, empire_id);

	clone->combatant = combatant;
	clone->region = region;
	clone->location = location;
	clone->value = value;
	clone->expanded = expanded;

	for (int i = 0; i < units.size(); i++) {
		CombatUnit* u = new CombatUnit(*units[i]);
		u->SetCombatGroup(clone);
		clone->units.append(u);
	}

	if (deep) {
		for (int i = 0; i < components.size(); i++) {
			CombatGroup* g = components[i]->Clone(deep);
			clone->AddComponent(g);

			if (g->GetType() == ECOMBATGROUP_TYPE::FIGHTER_SQUADRON ||
				g->GetType() == ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON ||
				g->GetType() == ECOMBATGROUP_TYPE::ATTACK_SQUADRON ||
				g->GetType() == ECOMBATGROUP_TYPE::LCA_SQUADRON) {

				if (units.size() > 0) {
					CombatUnit* carrier = units[0];

					for (int u = 0; u < g->GetUnits().size(); u++) {
						CombatUnit* unit = g->GetUnits()[u];

						if (unit->GetType() >= (int)CLASSIFICATION::FIGHTER ||
							unit->GetType() <= (int)CLASSIFICATION::LCA) {
							unit->SetCarrier(carrier);
							unit->SetRegion(carrier->GetRegion());
						}
					}
				}
			}
		}
	}

	return clone;
}

// +--------------------------------------------------------------------+

const char*
CombatGroup::GetOrdinal() const
{
	static char ordinal[16];

	int last_two_digits = id % 100;

	if (last_two_digits > 10 && last_two_digits < 20) {
		sprintf_s(ordinal, "ordinal.%d", last_two_digits);
		Text suffix = ordinal;

		if (suffix != ordinal)
			sprintf_s(ordinal, "%d%s", id, suffix.data());
		else
			sprintf_s(ordinal, "%dth", id);
	}
	else {
		int last_digit = last_two_digits % 10;
		sprintf_s(ordinal, "ordinal.%d", last_digit);
		Text suffix = ordinal;
		if (suffix != ordinal)
			sprintf_s(ordinal, "%d%s", id, suffix.data());
		else if (last_digit == 1)
			sprintf_s(ordinal, "%dst", id);
		else if (last_digit == 2)
			sprintf_s(ordinal, "%dnd", id);
		else if (last_digit == 3)
			sprintf_s(ordinal, "%drd", id);
		else
			sprintf_s(ordinal, "%dth", id);
	}

	return ordinal;
}

const char*
CombatGroup::GetDescription() const
{
	static char desc[256];
	static char name_desc[256];

	if (name.length())
		sprintf_s(name_desc, " \"%s\"", (const char*)name);
	else
		name_desc[0] = 0;

	switch (type) {
	case ECOMBATGROUP_TYPE::FORCE:				  strcpy_s(desc, (const char*)name); break;

	case ECOMBATGROUP_TYPE::FLEET:                sprintf_s(desc, "%s %s%s", GetOrdinal(), "Fleet", name_desc); break;
	case ECOMBATGROUP_TYPE::CARRIER_GROUP:        sprintf_s(desc, "%s %s%s", GetOrdinal(), "CVBG", name_desc); break;
	case ECOMBATGROUP_TYPE::BATTLE_GROUP:         sprintf_s(desc, "%s %s%s", GetOrdinal(), "BG", name_desc); break;
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:   sprintf_s(desc, "%s %s%s", GetOrdinal(), "DESRON", name_desc); break;

	case ECOMBATGROUP_TYPE::WING:                 sprintf_s(desc, "%s %s%s", GetOrdinal(), "WING", name_desc); break;
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:      sprintf_s(desc, "%s %s%s", GetOrdinal(), "VA", name_desc); break;
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:     sprintf_s(desc, "%s %s%s", GetOrdinal(), "VF", name_desc); break;
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:   sprintf_s(desc, "%s %s%s", GetOrdinal(), "INT", name_desc); break;
	case ECOMBATGROUP_TYPE::LCA_SQUADRON:         sprintf_s(desc, "%s %s%s", GetOrdinal(), "LCA", name_desc); break;

	case ECOMBATGROUP_TYPE::BATTALION:            sprintf_s(desc, "%s %s%s", GetOrdinal(), "BATTALION", name_desc); break;
	case ECOMBATGROUP_TYPE::STATION:              sprintf_s(desc, "%s %s", "STATION", name.data()); break;
	case ECOMBATGROUP_TYPE::STARBASE:             sprintf_s(desc, "%s %d%s","STARBASE", id, name_desc); break;
	case ECOMBATGROUP_TYPE::MINEFIELD:            sprintf_s(desc, "%s %s%s", GetOrdinal(), "MINEFIELD", name_desc); break;
	case ECOMBATGROUP_TYPE::BATTERY:              sprintf_s(desc, "%s %s%s", GetOrdinal(), "BATTERY", name_desc); break;
	case ECOMBATGROUP_TYPE::MISSILE:              sprintf_s(desc, "%s %s%s", GetOrdinal(), "MISSILE", name_desc); break;

	case ECOMBATGROUP_TYPE::C3I:                  sprintf_s(desc, "%s %s%s", GetOrdinal(), "C3I", name_desc); break;
	case ECOMBATGROUP_TYPE::COMM_RELAY:           sprintf_s(desc, "%s %s%s", GetOrdinal(), "COMM", name_desc); break;
	case ECOMBATGROUP_TYPE::EARLY_WARNING:        sprintf_s(desc, "%s %s%s", GetOrdinal(), "EW", name_desc); break;
	case ECOMBATGROUP_TYPE::FWD_CONTROL_CTR:      sprintf_s(desc, "%s %s%s", GetOrdinal(), "C3I", name_desc); break;
	case ECOMBATGROUP_TYPE::ECM:                  sprintf_s(desc, "%s %s%s", GetOrdinal(), "ECM", name_desc); break;

	case ECOMBATGROUP_TYPE::SUPPORT:              sprintf_s(desc, "%s %s%s", GetOrdinal(), "SUPPORT", name_desc); break;
	case ECOMBATGROUP_TYPE::COURIER:              sprintf_s(desc, "%s %s%s", GetOrdinal(), "COURIER", name_desc); break;
	case ECOMBATGROUP_TYPE::SUPPLY:               sprintf_s(desc, "%s %s%s", GetOrdinal(), "SUPPLY", name_desc); break;
	case ECOMBATGROUP_TYPE::REPAIR:               sprintf_s(desc, "%s %s%s", GetOrdinal(), "REPAIR", name_desc); break;
	case ECOMBATGROUP_TYPE::MEDICAL:              sprintf_s(desc, "%s %s%s", GetOrdinal(), "MEDICAL", name_desc); break;

	case ECOMBATGROUP_TYPE::CIVILIAN:
	case ECOMBATGROUP_TYPE::WAR_PRODUCTION:
	case ECOMBATGROUP_TYPE::FACTORY:
	case ECOMBATGROUP_TYPE::REFINERY:
	case ECOMBATGROUP_TYPE::RESOURCE:             strcpy_s(desc, (const char*)name); break;

	case ECOMBATGROUP_TYPE::INFRASTRUCTURE:
	case ECOMBATGROUP_TYPE::TRANSPORT:
	case ECOMBATGROUP_TYPE::NETWORK:
	case ECOMBATGROUP_TYPE::HABITAT:
	case ECOMBATGROUP_TYPE::STORAGE:
	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:              strcpy_s(desc, (const char*)name); break;

	default:                   sprintf_s(desc, "%s%s", "Default Combat Group", name_desc); break;
	}

	return desc;
}

const char*
CombatGroup::GetShortDescription() const
{
	static char desc[256];

	switch (type) {
	case ECOMBATGROUP_TYPE::FORCE:                strcpy_s(desc, (const char*)name); break;

	case ECOMBATGROUP_TYPE::FLEET:                sprintf_s(desc, "%s %s", GetOrdinal(), "Fleet"); break;
	case ECOMBATGROUP_TYPE::CARRIER_GROUP:        sprintf_s(desc, "%s %s", GetOrdinal(), "CVBG"); break;
	case ECOMBATGROUP_TYPE::BATTLE_GROUP:         sprintf_s(desc, "%s %s", GetOrdinal(), "BG"); break;
	case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:   sprintf_s(desc, "%s %s", GetOrdinal(), "DESRON"); break;

	case ECOMBATGROUP_TYPE::WING:                 sprintf_s(desc, "%s %s", GetOrdinal(), "Wing"); break;
	case ECOMBATGROUP_TYPE::ATTACK_SQUADRON:      sprintf_s(desc, "%s %s", GetOrdinal(), "AS"); break;
	case ECOMBATGROUP_TYPE::FIGHTER_SQUADRON:     sprintf_s(desc, "%s %s", GetOrdinal(), "FS"); break;
	case ECOMBATGROUP_TYPE::INTERCEPT_SQUADRON:   sprintf_s(desc, "%s %s", GetOrdinal(), "IS"); break;
	case ECOMBATGROUP_TYPE::LCA_SQUADRON:         sprintf_s(desc, "%s %s", GetOrdinal(), "LCA"); break;

	case ECOMBATGROUP_TYPE::BATTALION:            sprintf_s(desc, "%s %s", GetOrdinal(), "BN"); break;
	case ECOMBATGROUP_TYPE::STATION:              sprintf_s(desc, "%s %s", GetOrdinal(), "STN"); break;
	case ECOMBATGROUP_TYPE::STARBASE:             sprintf_s(desc, "%s %s", GetOrdinal(), "SB"); break;
	case ECOMBATGROUP_TYPE::MINEFIELD:            sprintf_s(desc, "%s %s", GetOrdinal(), "MF"); break;
	case ECOMBATGROUP_TYPE::BATTERY:              sprintf_s(desc, "%s %s", GetOrdinal(), "BTTY"); break;
	case ECOMBATGROUP_TYPE::C3I:                  sprintf_s(desc, "%s %s", GetOrdinal(), "C3I"); break;
	case ECOMBATGROUP_TYPE::COMM_RELAY:           sprintf_s(desc, "%s %s", GetOrdinal(), "RELAY"); break;
	case ECOMBATGROUP_TYPE::EARLY_WARNING:        sprintf_s(desc, "%s %s", GetOrdinal(), "EW"); break;
	case ECOMBATGROUP_TYPE::FWD_CONTROL_CTR:      sprintf_s(desc, "%s %s", GetOrdinal(), "FCC"); break;
	case ECOMBATGROUP_TYPE::ECM:                  sprintf_s(desc, "%s %s", GetOrdinal(), "ECM"); break;

	case ECOMBATGROUP_TYPE::SUPPORT:              sprintf_s(desc, "%s %s", GetOrdinal(), "SPT"); break;
	case ECOMBATGROUP_TYPE::COURIER:              sprintf_s(desc, "%s %s", GetOrdinal(), "TRN"); break;
	case ECOMBATGROUP_TYPE::MEDICAL:              sprintf_s(desc, "%s %s", GetOrdinal(), "MED"); break;
	case ECOMBATGROUP_TYPE::SUPPLY:               sprintf_s(desc, "%s %s", GetOrdinal(), "SUP"); break;
	case ECOMBATGROUP_TYPE::REPAIR:               sprintf_s(desc, "%s %s", GetOrdinal(), "REP"); break;

	case ECOMBATGROUP_TYPE::CIVILIAN:
	case ECOMBATGROUP_TYPE::WAR_PRODUCTION:
	case ECOMBATGROUP_TYPE::FACTORY:
	case ECOMBATGROUP_TYPE::REFINERY:
	case ECOMBATGROUP_TYPE::RESOURCE:             strcpy_s(desc, (const char*)name); break;

	case ECOMBATGROUP_TYPE::INFRASTRUCTURE:
	case ECOMBATGROUP_TYPE::TRANSPORT:
	case ECOMBATGROUP_TYPE::NETWORK:
	case ECOMBATGROUP_TYPE::HABITAT:
	case ECOMBATGROUP_TYPE::STORAGE:
	case ECOMBATGROUP_TYPE::FREIGHT:
	case ECOMBATGROUP_TYPE::PASSENGER:
	case ECOMBATGROUP_TYPE::PRIVATE:              strcpy_s(desc, (const char*)name); break;

	default:                   sprintf_s(desc, "%s", "Default Combat Group"); break;
	}

	return desc;
}

// +--------------------------------------------------------------------+

double
CombatGroup::GetNextJumpTime() const
{
	double t = 0;

	ListIter<CombatUnit> unit = ((CombatGroup*)this)->units;
	while (++unit)
		if (unit->GetNextJumpTime() > t)
			t = unit->GetNextJumpTime();

	return t;
}

// +--------------------------------------------------------------------+

void
CombatGroup::MoveTo(FVector& loc)
{
	location = loc;
}

// +--------------------------------------------------------------------+

void
CombatGroup::SetAssignedSystem(const char* s)
{
	assigned_system = s;
	assigned_zone = 0;
	zone_lock = false;

	ListIter<CombatGroup> iter = components;
	while (++iter) {
		CombatGroup* g = iter.value();
		g->SetAssignedSystem(s);
	}
}

void
CombatGroup::SetAssignedZone(CombatZone* z)
{
	assigned_zone = z;

	if (!assigned_zone)
		zone_lock = false;

	ListIter<CombatGroup> iter = components;
	while (++iter) {
		CombatGroup* g = iter.value();
		g->SetAssignedZone(z);
	}
}

void
CombatGroup::ClearUnlockedZones()
{
	if (!zone_lock)
		assigned_zone = 0;

	ListIter<CombatGroup> iter = components;
	while (++iter) {
		CombatGroup* g = iter.value();
		g->ClearUnlockedZones();
	}
}

void
CombatGroup::SetZoneLock(bool lock)
{
	if (!assigned_zone)
		zone_lock = false;
	else
		zone_lock = lock;

	if (zone_lock)
		assigned_system = Text();

	ListIter<CombatGroup> iter = components;
	while (++iter) {
		CombatGroup* g = iter.value();
		g->SetZoneLock(lock);
	}
}

// +--------------------------------------------------------------------+

void
CombatGroup::SetIntelLevel(int n)
{
	if (n < Intel::RESERVE || n > Intel::TRACKED) return;

	enemy_intel = n;

	// if this group has been discovered, the entire
	// branch of the OOB tree must be exposed.  Otherwise,
	// no missions would ever be planned against this
	// combat group.
	if (n > Intel::SECRET) {
		CombatGroup* p = parent;
		while (p) {
			if (p->enemy_intel < Intel::KNOWN)
				p->enemy_intel = Intel::KNOWN;
			p = p->parent;
		}
	}
}

// +--------------------------------------------------------------------+

int
CombatGroup::CalcValue()
{
	int val = 0;

	ListIter<CombatUnit> unit = units;
	while (++unit)
		val += unit->GetValue();

	ListIter<CombatGroup> comp = components;
	while (++comp)
		val += comp->CalcValue();

	value = val;
	return value;
}

int CombatGroup::CalcValue() const
{
	// Safe legacy-style: reuse the existing non-const implementation
	return const_cast<CombatGroup*>(this)->CalcValue();
}

int
CombatGroup::CountUnits() const
{
	int n = 0;

	CombatGroup* g = (CombatGroup*)this;

	ListIter<CombatUnit> unit = g->units;
	while (++unit)
		n += unit->GetCount() - unit->DeadCount();

	CombatGroup* pThis = ((CombatGroup*)this);
	pThis->live_comp.clear();

	ListIter<CombatGroup> iter = g->components;
	while (++iter) {
		CombatGroup* comp = iter.value();

		if (!comp->IsReserve()) {
			int unit_count = comp->CountUnits();
			if (unit_count > 0)
				pThis->live_comp.append(comp);

			n += unit_count;
		}
	}

	return n;
}

// +--------------------------------------------------------------------+

void
CombatGroup::ClearAssignments()
{
	assignments.destroy();

	ListIter<CombatGroup> comp = components;
	while (++comp)
		comp->ClearAssignments();
}

// +--------------------------------------------------------------------+

CombatGroup*
CombatGroup::FindCarrier()
{
	CombatGroup* p = GetParent();

	while (p != 0 &&
		p->GetType() != ECOMBATGROUP_TYPE::CARRIER_GROUP &&
		p->GetType() != ECOMBATGROUP_TYPE::STATION &&
		p->GetType() != ECOMBATGROUP_TYPE::STARBASE)
		p = p->GetParent();

	if (p && p->GetUnits().size())
		return p;

	return 0;
}

CombatUnit*
CombatGroup::GetRandomUnit()
{
	CombatUnit* result = 0;
	List<CombatUnit> live;

	ListIter<CombatUnit> unit = units;
	while (++unit) {
		if (unit->GetCount() - unit->DeadCount() > 0)
			live.append(unit.value());
	}

	if (live.size() > 0) {
		int ntries = 5;
		while (!result && ntries-- > 0) {
			int index = FMath::Rand() % live.size();
			result = live[index];

			int ship_class = result->GetShipClass();
			if (ship_class >= (int)CLASSIFICATION::CRUISER &&
				ship_class <= (int)CLASSIFICATION::FARCASTER)
				result = 0;
		}
	}

	if (!result) {
		ListIter<CombatGroup> comp = components;
		while (++comp && !result) {
			CombatUnit* u = comp->GetRandomUnit();
			if (u)
				result = u;
		}
	}

	return result;
}

CombatUnit*
CombatGroup::GetFirstUnit()
{
	int tmp_index = unit_index;
	unit_index = 0;
	CombatUnit* result = GetNextUnit();
	unit_index = tmp_index;

	return result;
}

CombatUnit*
CombatGroup::GetNextUnit()
{
	if (units.size() > 0) {
		List<CombatUnit> live;

		ListIter<CombatUnit> unit = units;
		while (++unit) {
			if (unit->GetCount() - unit->DeadCount() > 0)
				live.append(unit.value());
		}

		if (live.size() > 0) {
			return live[unit_index++ % live.size()];
		}
	}

	if (components.size() > 0) {
		return components[unit_index % components.size()]->GetNextUnit();
	}

	return 0;
}

CombatUnit*
CombatGroup::FindUnit(const char* iname)
{
	if (units.size() > 0) {
		ListIter<CombatUnit> iter = units;
		while (++iter) {
			CombatUnit* unit = iter.value();
			if (unit->GetName() == iname) {
				if (unit->GetCount() - unit->DeadCount() > 0)
					return unit;
				else
					return 0;
			}
		}
	}

	return 0;
}

void
CombatGroup::AssignRegion(Text rgn)
{
	region = rgn;

	ListIter<CombatGroup> comp = components;
	while (++comp)
		comp->AssignRegion(rgn);

	ListIter<CombatUnit> unit = units;
	while (++unit)
		unit->SetRegion(rgn);
}

// +--------------------------------------------------------------------+

static const char* group_name[] = {
	"",
	"force",
	"wing",
	"intercept_squadron",
	"fighter_squadron",
	"attack_squadron",
	"lca_squadron",
	"fleet",
	"destroyer_squadron",
	"battle_group",
	"carrier_group",
	"battalion",
	"minefield",
	"battery",
	"missile",
	"station",
	"starbase",
	"c3i",
	"comm_relay",
	"early_warning",
	"fwd_control_ctr",
	"ecm",
	"support",
	"courier",
	"medical",
	"supply",
	"repair",
	"civilian",
	"war_production",
	"factory",
	"refinery",
	"resource",
	"infrastructure",
	"transport",
	"network",
	"habitat",
	"storage",
	"non_com",
	"freight",
	"passenger",
	"private"
};

// +--------------------------------------------------------------------+

ECOMBATGROUP_TYPE
CombatGroup::TypeFromName(const char* type_name)
{
	if (!type_name || !type_name[0])
		return ECOMBATGROUP_TYPE::UNKNOWN;

	for (int i = (int)ECOMBATGROUP_TYPE::FORCE;
		i < (int)ECOMBATGROUP_TYPE::PRIVATE;
		i++)
	{
		if (!_stricmp(type_name, group_name[i]))
			return static_cast<ECOMBATGROUP_TYPE>(i);
	}

	return ECOMBATGROUP_TYPE::UNKNOWN;
}

const char*
CombatGroup::NameFromType(ECOMBATGROUP_TYPE type)
{
	const int index = static_cast<int>(type);

	if (index >= (int) ECOMBATGROUP_TYPE::FORCE && index < (int) ECOMBATGROUP_TYPE::PRIVATE)
		return group_name[index];

	return "UNKNOWN";
}
// +--------------------------------------------------------------------+

int ShipClassFromName(const char* type_name)
{
	return Ship::ClassForName(type_name);
}

// +--------------------------------------------------------------------+

#define GET_DEF_BOOL(n) if (pdef->name()->value()==(#n)) GetDefBool((n),   pdef, filename)
#define GET_DEF_TEXT(n) if (pdef->name()->value()==(#n)) GetDefText((n),   pdef, filename)
#define GET_DEF_NUM(n)  if (pdef->name()->value()==(#n)) GetDefNumber((n), pdef, filename)
#define GET_DEF_VEC(n)  if (pdef->name()->value()==(#n)) GetDefVec((n),    pdef, filename)

// NOTE:
// The parsing/loading and save-game merge logic below is intentionally kept structurally identical
// to the original Starshatter code (same locals, flow, and side-effects). It depends on your
// existing shims for Parser/Term/TermDef/TermStruct/DataLoader/BlockReader/GetDefX helpers.

CombatGroup*
CombatGroup::LoadOrderOfBattle(const char* filename, int team, Combatant* combatant)
{
	CombatGroup* force = 0;
	DataLoader* loader = DataLoader::GetLoader();
	BYTE* block;

	loader->LoadBuffer(filename, block, true);

	Parser parser(new BlockReader((const char*)block));
	Term* term = parser.ParseTerm();

	if (!term) {
		Print(TEXT("ERROR: could not parse order of battle '%s'\n"), filename);
		return 0;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "ORDER_OF_BATTLE") {
			Print(TEXT("ERROR: invalid Order of Battle file '%s'\n"), filename);
			term->print(10);
			return 0;
		}
	}

	do {
		delete term; term = 0;
		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				if (def->name()->value() == "group") {
					if (!def->term() || !def->term()->isStruct()) {
						Print(TEXT("WARNING: group struct missing in '%s'\n"), filename);
					}
					else {
						TermStruct* val = def->term()->isStruct();

						char  name[256];
						char  type[64];
						char  intel[64];
						char  region[64];
						char  system[64];
						char  parent_type[64];
						int   parent_id = 0;
						int   id = 0;
						int   iff = -1;
						Vec3  loc = Vec3(1.0e9f, 0.0f, 0.0f);
						EEMPIRE_NAME empire = EEMPIRE_NAME::Terellian;
						List<CombatUnit>  unit_list;
						char              unit_name[64];
						char              unit_regnum[16];
						char              unit_design[64];
						char              unit_skin[64];
						int               unit_class = 0;
						int               unit_count = 1;
						int               unit_dead = 0;
						int               unit_damage = 0;
						int               unit_heading = 0;
						int               unit_index = 0;

						*name = 0;
						*type = 0;
						*intel = 0;
						*region = 0;
						*system = 0;
						*parent_type = 0;
						*unit_name = 0;
						*unit_regnum = 0;
						*unit_design = 0;
						*unit_skin = 0;

						strcpy_s(intel, "KNOWN");

						// all groups in this OOB default to the IFF of the main force
						if (force)
							iff = force->GetIFF();

						for (int i = 0; i < val->elements()->size(); i++) {
							TermDef* pdef = val->elements()->at(i)->isDef();
							if (pdef && (iff < 0 || team < 0 || iff == team)) {
								GET_DEF_TEXT(name);
							else GET_DEF_TEXT(type);
						else GET_DEF_TEXT(intel);
				else GET_DEF_TEXT(region);
			else GET_DEF_TEXT(system);
		else GET_DEF_VEC(loc);
else GET_DEF_TEXT(parent_type);
else GET_DEF_NUM(parent_id);
else GET_DEF_NUM(iff);
else GET_DEF_NUM(id);
								else GET_DEF_NUM(unit_index);

								else if ((iff == team || team < 0) && pdef->name()->value() == "unit") {
									if (!pdef->term() || !pdef->term()->isStruct()) {
										Print(TEXT("WARNING: unit struct missing for group '%s' in '%s'\n"), name, filename);
									}
									else {
										TermStruct* valx = pdef->term()->isStruct();

										char unit_region[64];
										char design[256];
										FVector unit_loc = FVector(1.0e9f, 0.0f, 0.0f);
										unit_count = 1;

										FMemory::Memzero(unit_region, sizeof(unit_region));
										FMemory::Memzero(design, sizeof(design));

										for (int ix = 0; ix < valx->elements()->size(); ix++) {
											TermDef* pdefx = valx->elements()->at(ix)->isDef();
											if (pdefx) {
												if (pdefx->name()->value() == "name") {
													GetDefText(unit_name, pdefx, filename);
												}
												else if (pdefx->name()->value() == "regnum") {
													GetDefText(unit_regnum, pdefx, filename);
												}
												else if (pdefx->name()->value() == "region") {
													GetDefText(unit_region, pdefx, filename);
												}
												else if (pdefx->name()->value() == "loc") {
													Vec3 temp; 
													GetDefVec(temp, pdefx, filename);
													unit_loc = FVector(temp.X, temp.Y, temp.Z); 
												}
												else if (pdefx->name()->value() == "type") {
													char typestr[32];
													GetDefText(typestr, pdefx, filename);
													unit_class = ShipDesign::ClassForName(typestr);
												}
												else if (pdefx->name()->value() == "design") {
													GetDefText(unit_design, pdefx, filename);
												}
												else if (pdefx->name()->value() == "skin") {
													GetDefText(unit_skin, pdefx, filename);
												}
												else if (pdefx->name()->value() == "count") {
													GetDefNumber(unit_count, pdefx, filename);
												}
												else if (pdefx->name()->value() == "dead_count") {
													GetDefNumber(unit_dead, pdefx, filename);
												}
												else if (pdefx->name()->value() == "damage") {
													GetDefNumber(unit_damage, pdefx, filename);
												}
												else if (pdefx->name()->value() == "heading") {
													GetDefNumber(unit_heading, pdefx, filename);
												}
											}
										}

										if (!ShipDesign::CheckName(unit_design)) {
											Print(TEXT("ERROR: invalid design '%s' for unit '%s' in '%s'\n"), unit_design, unit_name, filename);
											return 0;
										}

										CombatUnit* cu = new CombatUnit(unit_name, unit_regnum, unit_class, unit_design, unit_count, iff);
										cu->SetRegion(unit_region);
										cu->SetSkin(unit_skin);
										cu->MoveTo(unit_loc);
										cu->GetKill(unit_dead);
										cu->SetSustainedDamage(unit_damage);
										cu->SetHeading(unit_heading * DEGREES);
										unit_list.append(cu);
									}
								}
							}
						}  // elements

						if (iff >= 0 && (iff == team || team < 0)) {
							CombatGroup* parent_group = 0;

							if (force) {
								parent_group = force->FindGroup(TypeFromName(parent_type), parent_id);
							}

							empire = EEMPIRE_NAME::Terellian;
							CombatGroup* g = new
								CombatGroup(TypeFromName(type), id, name, iff, Intel::IntelFromName(intel), empire, parent_group);

							g->region = region;
							g->combatant = combatant;
							g->unit_index = unit_index;

							if (loc.X >= 1e9) {
								if (parent_group)
									g->location = parent_group->location;
								else
									g->location = FVector::Zero();
							}
							else {
								g->location = FVector(loc.X, loc.Y, loc.Z);
							}

							if (unit_list.size()) {
								unit_list[0]->SetLeader(true);

								ListIter<CombatUnit> u = unit_list;
								while (++u) {
									u->SetCombatGroup(g);

									if (u->GetRegion().length() < 1) {
										u->SetRegion(g->GetRegion());
										u->MoveTo(g->GetLocation());
									}

									if (parent_group &&
										(u->GetType() == (int)CLASSIFICATION::FIGHTER ||
											u->GetType() == (int)CLASSIFICATION::ATTACK)) {

										CombatUnit* carrier = 0;
										CombatGroup* p = parent_group;

										while (p && !carrier) {
											if (p->units.size() && p->units[0]->GetType() == (int)CLASSIFICATION::CARRIER) {
												carrier = p->units[0];
												u->SetCarrier(carrier);
												u->SetRegion(carrier->GetRegion());
											}

											p = p->parent;
										}
									}
								}

								g->units.append(unit_list);
							}

							if (!force)
								force = g;
						}  // iff == team?
					}     // group-struct
				}        // group
			}           // def
		}              // term
	} while (term);

	loader->ReleaseBuffer(block);
	Print(TEXT("Order of Battle Loaded (%s).\n"), force ? force->GetName().data() : "unknown force");

	if (force)
		force->CalcValue();

	return force;
}

// +--------------------------------------------------------------------+

void
CombatGroup::MergeOrderOfBattle(BYTE* block, const char* filename, int team, Combatant* combatant, Campaign* campaign)
{
	CombatGroup* force = 0;

	Parser parser(new BlockReader((const char*)block));
	Term* term = parser.ParseTerm();

	if (!term) {
		Print(TEXT("ERROR: could notxparse order of battle '%s'\n"), filename);
		return;
	}
	else {
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "SAVEGAME") {
			Print(TEXT("ERROR: invalid Save Game file '%s'\n"), filename);
			term->print(10);
			return;
		}
	}

	do {
		delete term; term = 0;
		term = parser.ParseTerm();

		if (term) {
			TermDef* def = term->isDef();
			if (def) {
				if (def->name()->value() == "group") {
					if (!def->term() || !def->term()->isStruct()) {
						Print(TEXT("WARNING: group struct missing in '%s'\n"), filename);
					}
					else {
						TermStruct* val = def->term()->isStruct();

						char  name[256];
						char  type[64];
						char  intel[64];
						char  region[64];
						char  system[64];
						char  zone[64];
						bool  zone_locked = false;
						int   id = 0;
						int   iff = -1;
						int   sorties = -1;
						int   kills = -1;
						int   points = -1;
						Vec3  loc = Vec3(1.0e9f, 0.0f, 0.0f);

						List<CombatUnit>  unit_list;
						char              unit_name[64];
						char              unit_regnum[16];
						char              unit_design[64];
						int               unit_class = 0;
						int               unit_count = 1;
						int               unit_dead = 0;
						int               unit_damage = 0;
						int               unit_heading = 0;
						int               unit_index = 0;

						*name = 0;
						*type = 0;
						*intel = 0;
						*region = 0;
						*system = 0;
						*zone = 0;
						*unit_name = 0;
						*unit_regnum = 0;
						*unit_design = 0;

						strcpy_s(intel, "KNOWN");

						// all groups in this OOB default to the IFF of the main force
						if (force)
							iff = force->GetIFF();

						for (int i = 0; i < val->elements()->size(); i++) {
							TermDef* pdef = val->elements()->at(i)->isDef();
							if (pdef && (iff < 0 || team < 0 || iff == team)) {
								GET_DEF_TEXT(name);
							else GET_DEF_TEXT(type);
						else GET_DEF_TEXT(intel);
				else GET_DEF_TEXT(region);
			else GET_DEF_TEXT(system);
		else GET_DEF_TEXT(zone);
else GET_DEF_BOOL(zone_locked);
else GET_DEF_VEC(loc);
else GET_DEF_NUM(iff);
else GET_DEF_NUM(id);
								else GET_DEF_NUM(sorties);
								else GET_DEF_NUM(kills);
								else GET_DEF_NUM(points);
								else GET_DEF_NUM(unit_index);

								else if ((iff == team || team < 0) && pdef->name()->value() == "unit") {
									if (!pdef->term() || !pdef->term()->isStruct()) {
										Print(TEXT("WARNING: unit struct missing for group '%s' in '%s'\n"), name, filename);
									}
									else {
										TermStruct* valu = pdef->term()->isStruct();

										char unit_region[64];
										char design[256];
										FVector unit_loc = FVector::ZeroVector;
										unit_count = 1;

										FMemory::Memzero(unit_region, sizeof(unit_region));
										FMemory::Memzero(design, sizeof(design));

										for (int iu = 0; iu < valu->elements()->size(); iu++) {
											TermDef* pdefu = valu->elements()->at(iu)->isDef();
											if (pdefu) {
												if (pdefu->name()->value() == "name") {
													GetDefText(unit_name, pdefu, filename);
												}
												else if (pdefu->name()->value() == "regnum") {
													GetDefText(unit_regnum, pdefu, filename);
												}
												else if (pdefu->name()->value() == "region") {
													GetDefText(unit_region, pdefu, filename);
												}
												else if (pdefu->name()->value() == "loc") {
													Vec3 temp;
													GetDefVec(temp, pdefu, filename);
													unit_loc = FVector(temp.X, temp.Y, temp.Z);
												}
												else if (pdefu->name()->value() == "type") {
													char typestr[32];
													GetDefText(typestr, pdefu, filename);
													unit_class = ShipDesign::ClassForName(typestr);
												}
												else if (pdefu->name()->value() == "design") {
													GetDefText(unit_design, pdefu, filename);
												}
												else if (pdefu->name()->value() == "count") {
													GetDefNumber(unit_count, pdefu, filename);
												}
												else if (pdefu->name()->value() == "dead_count") {
													GetDefNumber(unit_dead, pdefu, filename);
												}
												else if (pdefu->name()->value() == "damage") {
													GetDefNumber(unit_damage, pdefu, filename);
												}
												else if (pdefu->name()->value() == "heading") {
													GetDefNumber(unit_heading, pdefu, filename);
												}
											}
										}

										if (!ShipDesign::CheckName(unit_design)) {
											Print(TEXT("ERROR: invalid design '%s' for unit '%s' in '%s'\n"), unit_design, unit_name, filename);
											return;
										}

										if (force) {
											CombatUnit* cu = new CombatUnit(unit_name, unit_regnum, unit_class, unit_design, unit_count, iff);
											cu->SetRegion(unit_region);
											cu->MoveTo(unit_loc);
											cu->GetKill(unit_dead);
											cu->SetSustainedDamage(unit_damage);
											cu->SetHeading(unit_heading * DEGREES);
											unit_list.append(cu);
										}
									}
									}
							}
						}  // elements

						if (iff >= 0 && (iff == team || team < 0)) {
							// have we found the force group we are looking for yet ?
							if (!force)
							{
								if (combatant &&
									FCString::Strcmp(
										ANSI_TO_TCHAR(combatant->GetName()),
										ANSI_TO_TCHAR(name)) == 0)
								{
									force = combatant->GetForce();
								}
							}
							else
							{
								// if we already have a force, and we find a second one,
								// it must be the start of a different combatant.
								// So don't process any further:
								if (TypeFromName(type) == ECOMBATGROUP_TYPE::FORCE)
								{
									break;
								}
							}

							CombatGroup* g = force->FindGroup(TypeFromName(type), id);

							if (!g) {
								::Print(TEXT("WARNING: unexpected combat group %s %d '%s' in '%s'\n"), type, id, name, filename);
								continue;
							}

							g->region = region;
							g->combatant = combatant;
							g->location = FVector(loc.X, loc.Y, loc.Z);
							g->enemy_intel = Intel::IntelFromName(intel);
							g->unit_index = unit_index;

							if (*zone) {
								CombatZone* combat_zone = campaign->GetZone(zone);

								if (combat_zone) {
									g->SetAssignedZone(combat_zone);
									g->SetZoneLock(zone_locked);
								}
								else {
									::Print(TEXT("WARNING: could notxfind combat zone '%s' for group %s %d '%s' in '%s'\n"), zone, type, id, name, filename);
								}
							}
							else if (*system) {
								g->SetAssignedSystem(system);
							}

							if (sorties >= 0) g->SetSorties(sorties);
							if (kills >= 0) g->SetKills(kills);
							if (points >= 0) g->SetPoints(points);

							if (unit_list.size()) {
								ListIter<CombatUnit> u_iter = unit_list;
								while (++u_iter) {
									CombatUnit* load_unit = u_iter.value();
									CombatUnit* u = g->FindUnit(load_unit->GetName());

									if (u) {
										if (load_unit->GetRegion().length() > 0) {
											u->SetRegion(load_unit->GetRegion());
											u->MoveTo(load_unit->GetLocation());
										}
										else {
											u->SetRegion(g->GetRegion());
											u->MoveTo(g->GetLocation());
										}
										u->SetDeadCount(load_unit->DeadCount());
										u->SetSustainedDamage(load_unit->GetSustainedDamage());
										u->SetHeading(load_unit->GetHeading());
									}
								}

								unit_list.destroy();
							}

							if (!force)
								force = g;
						}  // iff == team?
					}     // group-struct
				}        // group
			}           // def
		}              // term
	} while (term);

	Print(TEXT("Order of Battle Loaded (%s).\n"), force ? force->GetName().data() : "unknown force");

	if (force)
		force->CalcValue();
}

// +--------------------------------------------------------------------+

Text FormatNumber(double n)
{
	char buffer[64];

	if (fabs(n) < 1000)
		sprintf_s(buffer, "%d", (int)n);

	else if (fabs(n) < 1e6) {
		int nn = (int)n / 1000;
		sprintf_s(buffer, "%de3", nn);
	}

	else
		sprintf_s(buffer, "%g", n);

	return buffer;
}

void
SaveCombatUnit(FILE* f, CombatUnit* u)
{
	int type = u->GetType();

	if (type == 0 && u->GetDesign())
		type = u->GetDesign()->type;

	fprintf(f, "\n unit: {");
	fprintf(f, " name: \"%s\",", u->GetName().data());
	fprintf(f, " type: \"%s\",", Ship::GetShipClassName(type));
	fprintf(f, " design: \"%s\",", u->GetDesignName().data());

	if (u->GetCount() > 1) {
		fprintf(f, " count: %d,", u->GetCount());
	}
	else {
		fprintf(f, " regnum:\"%s\",", u->GetRegistryNumber().data());
	}

	if (u->GetRegion().length() > 0) {
		fprintf(f, " region:\"%s\",", u->GetRegion().data());

		Text x = FormatNumber(u->GetLocation().X);
		Text y = FormatNumber(u->GetLocation().Y);
		Text z = FormatNumber(u->GetLocation().Z);

		fprintf(f, " loc:(%s, %s, %s),", x.data(), y.data(), z.data());
	}

	fprintf(f, " dead_count: %d, damage: %d, heading: %d },",
		(int)u->DeadCount(),
		(int)u->GetSustainedDamage(),
		(int)(u->GetHeading() / DEGREES));
}

void
SaveCombatGroup(FILE* f, CombatGroup* g)
{
	fprintf(f, "group: {");
	fprintf(f, " type: %s,", CombatGroup::NameFromType(g->GetType()));
	fprintf(f, " id: %d,", g->GetID());
	fprintf(f, " name: \"%s\",", g->GetName().data());
	fprintf(f, " intel: %s,", Intel::NameFromIntel(g->GetIntelLevel()));
	fprintf(f, " iff: %d,", g->GetIFF());
	fprintf(f, " unit_index: %d,", g->GetUnitIndex());

	if (g->GetRegion().length()) {
		fprintf(f, " region:\"%s\",", g->GetRegion().data());
	}

	if (g->GetAssignedSystem().length()) {
		fprintf(f, " system: \"%s\",", g->GetAssignedSystem().data());
	}

	if (g->GetAssignedZone()) {
		fwprintf(
			f,
			L" zone: \"%s\",",
			ANSI_TO_TCHAR(g->GetAssignedZone()->GetName())
		);
		if (g->IsZoneLocked()) {
			fprintf(f, " zone_locked: true,");
		}
	}

	Text x = FormatNumber(g->GetLocation().X);
	Text y = FormatNumber(g->GetLocation().Y);
	Text z = FormatNumber(g->GetLocation().Z);

	fprintf(f, " loc: (%s, %s, %s),", x.data(), y.data(), z.data());

	CombatGroup* parent = g->GetParent();
	if (parent) {
		fprintf(f, " parent_type:%s,", CombatGroup::NameFromType(parent->GetType()));
		fprintf(f, " parent_id:%d,", parent->GetID());
	}

	fprintf(f, " sorties: %d,", g->GetSorties());
	fprintf(f, " kills: %d,", g->GetKills());
	fprintf(f, " points: %d,", g->GetPoints());

	ListIter<CombatUnit> u = g->GetUnits();
	while (++u) {
		SaveCombatUnit(f, u.value());
	}

	fprintf(f, " }\n");

	ListIter<CombatGroup> c = g->GetComponents();
	while (++c) {
		SaveCombatGroup(f, c.value());
	}
}

void
CombatGroup::SaveOrderOfBattle(const char* filename, CombatGroup* force)
{
	FILE* f;
	::fopen_s(&f, filename, "a+");

	if (f) {
		SaveCombatGroup(f, force);
		fprintf(f, "\n");
		fclose(f);
	}
}
