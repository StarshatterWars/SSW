/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimEvent.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simulation Events for mission summary
*/

#include "SimEvent.h"
#include "Sim.h"
#include "Game.h"

// Minimal Unreal logging support:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimEvent, Log, All);

// +====================================================================+

List<ShipStats>   records;

// +====================================================================+

SimEvent::SimEvent(ESimEvent e, const char* t, const char* i)
	: event(e), count(0)
{
	Sim* sim = Sim::GetSim();
	if (sim) {
		time = (int)sim->MissionClock();
	}
	else {
		time = (int)(Game::GetGameTime() / 1000);
	}

	SetTarget(t);
	SetInfo(i);
}

SimEvent::~SimEvent()
{
}

// +--------------------------------------------------------------------+

void
SimEvent::SetTime(int t)
{
	time = t;
}

void
SimEvent::SetTarget(const char* t)
{
	if (t && t[0])
		target = t;
}

void
SimEvent::SetInfo(const char* i)
{
	if (i && i[0])
		info = i;
}

void
SimEvent::SetCount(int c)
{
	count = c;
}

Text
SimEvent::GetEventDesc() const
{
	switch (event) {
	case ESimEvent::LAUNCH:         return "Launch";
	case ESimEvent::DOCK:           return "Dock";
	case ESimEvent::LAND:           return "Land";
	case ESimEvent::EJECT:          return "Eject";
	case ESimEvent::CRASH:          return "Crash";
	case ESimEvent::COLLIDE:        return "Collision With";
	case ESimEvent::DESTROYED:      return "Destroyed By";
	case ESimEvent::MAKE_ORBIT:     return "Make Orbit";
	case ESimEvent::BREAK_ORBIT:    return "Break Orbit";
	case ESimEvent::QUANTUM_JUMP:   return "Quantum Jump";
	case ESimEvent::LAUNCH_SHIP:    return "Launch Ship";
	case ESimEvent::RECOVER_SHIP:   return "Recover Ship";
	case ESimEvent::FIRE_GUNS:      return "Fire Guns";
	case ESimEvent::FIRE_MISSILE:   return "Fire Missile";
	case ESimEvent::DROP_DECOY:     return "Drop Decoy";
	case ESimEvent::GUNS_KILL:      return "Guns Kill";
	case ESimEvent::MISSILE_KILL:   return "Missile Kill";
	case ESimEvent::LAUNCH_PROBE:   return "Launch Probe";
	case ESimEvent::SCAN_TARGET:    return "Scan Target";
	default:             return "No Event";
	}
}

// +====================================================================+

ShipStats::ShipStats(const char* n, int i)
	: name(n), iff(i), kill1(0), kill2(0), lost(0), coll(0), points(0),
	cmd_points(0), gun_shots(0), gun_hits(0), missile_shots(0), missile_hits(0),
	combat_group(0), combat_unit(0), player(false), ship_class(0), elem_index(-1)
{
	if (!n || !n[0])
		name = "Unknown";
}

ShipStats::~ShipStats()
{
	events.destroy();
}

// +--------------------------------------------------------------------+

void
ShipStats::SetType(const char* t)
{
	if (t && t[0])
		type = t;
}

void
ShipStats::SetRole(const char* r)
{
	if (r && r[0])
		role = r;
}

void
ShipStats::SetRegion(const char* r)
{
	if (r && r[0])
		region = r;
}

void
ShipStats::SetCombatGroup(CombatGroup* g)
{
	combat_group = g;
}

void
ShipStats::SetCombatUnit(CombatUnit* u)
{
	combat_unit = u;
}

void
ShipStats::SetElementIndex(int n)
{
	elem_index = n;
}

void
ShipStats::SetPlayer(bool p)
{
	player = p;
}

// +--------------------------------------------------------------------+

void
ShipStats::Summarize()
{
	kill1 = 0;
	kill2 = 0;
	lost = 0;
	coll = 0;

	ListIter<SimEvent> iter = events;
	while (++iter) {
		SimEvent* event = iter.value();
		ESimEvent code = event->GetEvent();

		if (code == ESimEvent::GUNS_KILL)
			kill1++;

		else if (code == ESimEvent::MISSILE_KILL)
			kill2++;

		else if (code == ESimEvent::DESTROYED)
			lost++;

		else if (code == ESimEvent::CRASH)
			coll++;

		else if (code == ESimEvent::COLLIDE)
			coll++;
	}
}

// +--------------------------------------------------------------------+

SimEvent*
ShipStats::AddEvent(SimEvent* e)
{
	events.append(e);
	return e;
}

SimEvent*
ShipStats::AddEvent(ESimEvent event, const char* tgt, const char* info)
{
	SimEvent* e = new SimEvent(event, tgt, info);
	events.append(e);
	return e;
}

bool
ShipStats::HasEvent(ESimEvent event)
{
	for (int i = 0; i < events.size(); i++)
		if (events[i]->GetEvent() == event)
			return true;

	return false;
}

// +--------------------------------------------------------------------+

void ShipStats::Initialize() { records.destroy(); }
void ShipStats::Close() { records.destroy(); }

// +--------------------------------------------------------------------+

int
ShipStats::NumStats()
{
	return records.size();
}

ShipStats*
ShipStats::GetStats(int i)
{
	if (i >= 0 && i < records.size())
		return records.at(i);

	return 0;
}

ShipStats*
ShipStats::Find(const char* name)
{
	if (name && name[0]) {
		ListIter<ShipStats> iter = records;
		while (++iter) {
			ShipStats* stats = iter.value();
			if (!strcmp(stats->GetName(), name))
				return stats;
		}

		ShipStats* stats = new ShipStats(name);
		records.append(stats);
		return stats;
	}

	return 0;
}
