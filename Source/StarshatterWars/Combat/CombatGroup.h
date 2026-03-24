/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004.

	SUBSYSTEM:    Stars.exe
	FILE:         CombatGroup.h
	AUTHOR:       John DiCamillo

	UNREAL PORT:
	- Maintains all variables and methods (names, signatures, members).
	- Uses UE-compatible shims for Text, List, Point, etc. (Types.h / Geometry.h / Text.h / List.h / Intel.h).
*/

#pragma once

// Original includes mapped to Unreal-compatible shims:
#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "List.h"
#include "Intel.h"
#include "GameStructs.h"

// Minimal Unreal include (required for by-value FVector in the public API):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Campaign;
class Combatant;
class CombatGroup;
class CombatUnit;
class CombatZone;
class CombatAssignment;

// +--------------------------------------------------------------------+

class CombatGroup
{
public:
	static const char* TYPENAME() { return "CombatGroup"; }

	CombatGroup(ECOMBATGROUP_TYPE t, int n, const char* s, int i, int e, CombatGroup* p = 0);
	~CombatGroup();

	// comparison operators are used to sort combat groups into a priority list
	// in DESCENDING order, so the sense of the comparison is backwards from
	// usual...
	int operator <  (const CombatGroup& g) const { return value > g.value; }
	int operator <= (const CombatGroup& g) const { return value >= g.value; }
	int operator == (const CombatGroup& g) const { return this == &g; }

	// operations:
	static CombatGroup* LoadOrderOfBattle(const char* fname, int iff, Combatant* combatant);
	static void         SaveOrderOfBattle(const char* fname, CombatGroup* force);
	static void         MergeOrderOfBattle(BYTE* block, const char* fname, int iff, Combatant* combatant, Campaign* campaign);

	void                AddComponent(CombatGroup* g);
	CombatGroup* FindGroup(ECOMBATGROUP_TYPE t, int n = -1);
	CombatGroup* Clone(bool deep = true);

	// accessors and mutators:
	const char* GetDescription()      const;
	const char* GetShortDescription() const;

	void                SetCombatant(Combatant* c) { combatant = c; }

	Combatant* GetCombatant() { return combatant; }
	CombatGroup* GetParent() { return parent; }
	List<CombatGroup>& GetComponents() { return components; }
	List<CombatGroup>& GetLiveComponents() { return live_comp; }
	List<CombatUnit>& GetUnits() { return units; }
	CombatUnit* GetRandomUnit();
	CombatUnit* GetFirstUnit();
	CombatUnit* GetNextUnit();
	CombatUnit* FindUnit(const char* name);
	CombatGroup* FindCarrier();

	const Text& Name()               const { return name; }
	ECOMBATGROUP_TYPE   GetType()     const { return type; }
	int                 CountUnits()         const;
	int                 IntelLevel()         const { return enemy_intel; }
	int                 GetID()              const { return id; }
	int                 GetIFF()             const { return iff; }
	FVector             Location()           const { return location; }
	void                MoveTo(FVector& loc);
	const				Text& GetRegion()          const { return region; }
	void                SetRegion(Text rgn) { region = rgn; }
	void                AssignRegion(Text rgn);
	int                 Value()              const { return value; }
	int                 Sorties()            const { return sorties; }
	void                SetSorties(int n) { sorties = n; }
	int                 Kills()              const { return kills; }
	void                SetKills(int n) { kills = n; }
	int                 Points()             const { return points; }
	void                SetPoints(int n) { points = n; }
	int                 UnitIndex()          const { return unit_index; }

	double              GetNextJumpTime()    const;

	double              GetPlanValue()       const { return plan_value; }
	void                SetPlanValue(double v) { plan_value = v; }

	bool                IsAssignable()       const;
	bool                IsTargetable()       const;
	bool                IsDefensible()       const;
	bool                IsStrikeTarget()     const;
	bool                IsMovable()          const;
	bool                IsFighterGroup()     const;
	bool                IsStarshipGroup()    const;
	bool                IsReserve()          const;

	
	int CalcValue();
	int CalcValue() const; 

	// these two methods return zero terminated arrays of
	// integers identifying the preferred assets for attack
	// or defense in priority order:

	static const int*	PreferredAttacker(ECOMBATGROUP_TYPE InType);
	static const int*	PreferredDefender(ECOMBATGROUP_TYPE InType);

	bool                IsExpanded()         const { return expanded; }
	void                SetExpanded(bool e) { expanded = e; }

	const Text&			GetAssignedSystem()  const { return assigned_system; }
	void                SetAssignedSystem(const char* s);
	CombatZone*			GetCurrentZone()     const { return current_zone; }
	void                SetCurrentZone(CombatZone* z) { current_zone = z; }
	CombatZone*			GetAssignedZone()    const { return assigned_zone; }
	void                SetAssignedZone(CombatZone* z);
	void                ClearUnlockedZones();
	bool                IsZoneLocked()       const { return assigned_zone && zone_lock; }
	void                SetZoneLock(bool lock = true);
	bool                IsSystemLocked()     const { return assigned_system.length() > 0; }

	const Text&			GetStrategicDirection()      const 
						{ return strategic_direction; }

	void                SetStrategicDirection(Text dir) 
						{ strategic_direction = dir; }

	void                SetIntelLevel(int n);

	List<CombatAssignment>& GetAssignments() { return assignments; }
	void                    ClearAssignments();

	static ECOMBATGROUP_TYPE TypeFromName(const char* type_name);
	static const char*		 NameFromType(ECOMBATGROUP_TYPE type);

private:
	const char* GetOrdinal() const;

	// attributes:
	ECOMBATGROUP_TYPE   type;
	int                 id;
	Text                name;
	int                 iff;
	int                 enemy_intel;

	double              plan_value; // scratch pad for plan modules

	List<CombatUnit>    units;
	List<CombatGroup>   components;
	List<CombatGroup>   live_comp;
	Combatant* combatant;
	CombatGroup* parent;
	Text                region;
	FVector             location;
	int                 value;
	int                 unit_index;

	int                 sorties;
	int                 kills;
	int                 points;

	bool                expanded;   // for tree control

	Text                assigned_system;
	CombatZone* current_zone;
	CombatZone* assigned_zone;
	bool                zone_lock;
	List<CombatAssignment> assignments;

	Text                strategic_direction;
};

// +--------------------------------------------------------------------+
