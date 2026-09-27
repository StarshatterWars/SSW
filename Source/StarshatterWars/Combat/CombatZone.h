/*  Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo, Destroyer Studios LLC
	Copyright (C) 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CombatZone.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	CombatZone is used by the dynamic campaign strategy
	and logistics algorithms to assign forces to locations
	within the campaign.  A CombatZone is a collection of
	closely related sectors, and the assets contained
	within them.
*/

#pragma once

#include "Types.h"
#include "Text.h"
#include "List.h"
#include "GameStructs.h"

// Minimal Unreal include needed for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class CombatGroup;
class CombatUnit;
class ZoneForce;

// +--------------------------------------------------------------------+

class CombatZone
{
public:
	static const char* TYPENAME() { return "CombatZone"; }

	CombatZone();
	~CombatZone();

	int operator==(const CombatZone& g) const { return this == &g; }

	const Text& GetName()       const { return name; }
	const Text& GetSystem()     const { return system; }

	void              SetName(const char* n) { name = n ? n : ""; }
	void              SetSystem(const char* s) { system = s ? s : ""; }

	void              AddGroup(CombatGroup* g);
	void              RemoveGroup(CombatGroup* g);
	bool              HasGroup(CombatGroup* g);

	void              AddRegion(const char* rgn);
	bool              HasRegion(const char* rgn);

	List<Text>& GetRegions() { return regions; }
	List<ZoneForce>& GetForces() { return forces; }

	ZoneForce* FindForce(int iff);
	ZoneForce* MakeForce(int iff);

	void              Clear();

	static List<CombatZone>& Load(const char* filename);

private:
	// attributes:
	Text            name;
	Text            system;
	List<Text>      regions;
	List<ZoneForce> forces;
};

// +--------------------------------------------------------------------+

class ZoneForce
{
public:
	ZoneForce(int i);

	int                GetIFF() { return iff; }
	List<CombatGroup>& GetGroups() { return groups; }
	List<CombatGroup>& GetTargetList() { return target_list; }
	List<CombatGroup>& GetDefendList() { return defend_list; }

	void               AddGroup(CombatGroup* g);
	void               RemoveGroup(CombatGroup* g);
	bool               HasGroup(CombatGroup* g);

	int                GetNeed(ECOMBATGROUP_TYPE group_type) const;
	void               SetNeed(ECOMBATGROUP_TYPE group_type, int needed);
	void               AddNeed(ECOMBATGROUP_TYPE group_type, int needed);

private:
	// attributes:
	int               iff;
	List<CombatGroup> groups;
	List<CombatGroup> defend_list;
	List<CombatGroup> target_list;
	int               need[8];
};

// +--------------------------------------------------------------------+
