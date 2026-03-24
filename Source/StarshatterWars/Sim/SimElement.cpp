/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars
	FILE:         SimElement.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Package Element (e.g. Flight) class implementation
*/

#include "SimElement.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioHandler.h"
#include "Sim.h"
#include "Ship.h"

#include "Game.h"

// Minimal Unreal include required for UE_LOG in a .cpp:
#include "Logging/LogMacros.h"

// +----------------------------------------------------------------------+

#ifndef LOG_SIM
#define LOG_SIM LogTemp
#endif

static int id_key = 1000;

SimElement::SimElement(const char* call_sign, int a_iff, int a_type)
	: id(id_key++)
	, iff(a_iff)
	, type(a_type)
	, player(0)
	, command_ai(1)
	, respawns(0)
	, intel(0)
	, name(call_sign)
	, count(0)
	, commander(0)
	, assignment(0)
	, carrier(0)
	, squadron()
	, combat_group(0)
	, combat_unit(0)
	, launch_time(0)
	, hold_time(0)
	, rogue(false)
	, playable(true)
	, zone_lock(false)
	, load{ 0 }
{
	if (!call_sign) {
		char buf[32];
		sprintf_s(buf, "Pkg %d", id);
		name = buf;
	}

	SetLoadout(0);
}

SimElement::~SimElement()
{
	flight_plan.destroy();
	objectives.destroy();
	instructions.destroy();

	for (int i = 0; i < ships.size(); i++)
		ships[i]->SetElement(0);

	respawns = 0;
}

// +----------------------------------------------------------------------+

int
SimElement::AddShip(Ship* ship, int index)
{
	if (ship && !ships.contains(ship)) {
		Observe(ship);

		if (index < 0) {
			ships.append(ship);
			index = ships.size();
		}
		else {
			ships.insert(ship, index - 1);
		}

		ship->SetElement(this);

		if (respawns < ship->RespawnCount())
			respawns = ship->RespawnCount();
	}

	return index;
}

void
SimElement::DelShip(Ship* ship)
{
	if (ship && ships.contains(ship)) {
		ships.remove(ship);
		ship->SetElement(0);

		if (ships.isEmpty())
			respawns = ship->RespawnCount();
	}
}

Ship*
SimElement::GetShip(int index)
{
	if (index >= 1 && index <= ships.size())
		return ships[index - 1];

	return 0;
}

int
SimElement::GetShipClass()
{
	if (ships.size())
		return (int) ships[0]->Class();

	return 0;
}

int
SimElement::FindIndex(const Ship* s)
{
	return ships.index(s) + 1;
}

bool
SimElement::Contains(const Ship* s)
{
	return ships.contains(s);
}

bool
SimElement::IsActive() const
{
	bool active = false;

	for (int i = 0; i < ships.size() && !active; i++) {
		Ship* s = ships[i];
		if (s->Life() && s->MissionClock())
			active = true;
	}

	return active;
}

bool
SimElement::IsFinished() const
{
	bool finished = false;

	if (launch_time > 0 && respawns < 1) {
		finished = true;

		if (ships.size() > 0) {
			for (int i = 0; i < ships.size() && finished; i++) {
				Ship* s = ships[i];
				if (s->RespawnCount() > 0 ||
					s->MissionClock() == 0 ||
					(s->Life() && !s->GetInbound()))
				{
					finished = false;
				}
			}
		}
	}

	return finished;
}

bool
SimElement::IsNetObserver() const
{
	bool observer = !IsSquadron();

	for (int i = 0; i < ships.size() && observer; i++) {
		Ship* s = ships[i];

		if (!s->IsNetObserver())
			observer = false;
	}

	return observer;
}

bool
SimElement::IsSquadron() const
{
	return count > 0;
}

bool
SimElement::IsStatic() const
{
	if (IsSquadron() || IsFinished())
		return false;

	const Ship* s = ships.at(0);
	if (s && s->IsStatic())
		return true;

	return false;
}

// +----------------------------------------------------------------------+

bool
SimElement::IsHostileTo(const Ship* s) const
{
	if (iff <= 0 || iff >= 100 || !s || launch_time == 0 || IsFinished())
		return false;

	if (IsSquadron())
		return false;

	if (s->IsRogue())
		return true;

	int s_iff = s->GetIFF();

	if (s_iff <= 0 || s_iff >= 100 || s_iff == iff)
		return false;

	if (ships.size() > 0 && ships[0]->GetRegion() != s->GetRegion())
		return false;

	return true;
}

bool
SimElement::IsHostileTo(int iff_code) const
{
	if (iff <= 0 || iff >= 100 || launch_time == 0 || IsFinished())
		return false;

	if (IsSquadron())
		return false;

	if (iff_code <= 0 || iff_code >= 100 || iff_code == iff)
		return false;

	return true;
}

bool
SimElement::IsObjectiveTargetOf(const Ship* s) const
{
	if (!s || launch_time == 0 || IsFinished())
		return false;

	const char* e_name = Name().data();
	int         e_len = Name().length();

	Instruction* orders = s->GetRadioOrders();
	if (orders && orders->GetAction() > INSTRUCTION_ACTION::SWEEP) {
		const char* o_name = orders->TargetName();
		int         o_len = 0;

		if (o_name && *o_name)
			o_len = (int)strlen(o_name);

		if (e_len < o_len)
			o_len = e_len;

		if (!strncmp(e_name, o_name, o_len))
			return true;
	}

	SimElement* elem = s->GetElement();
	if (elem) {
		for (int i = 0; i < elem->NumObjectives(); i++) {
			Instruction* obj = elem->GetObjective(i);

			if (obj) {
				const char* o_name = obj->TargetName();
				int         o_len = 0;

				if (o_name && *o_name)
					o_len = (int)strlen(o_name);

				if (e_len < o_len)
					o_len = e_len;

				if (!strncmp(e_name, o_name, o_len))
					return true;
			}
		}
	}

	return false;
}

// +----------------------------------------------------------------------+

void
SimElement::SetLaunchTime(DWORD t)
{
	if (launch_time == 0 || t == 0)
		launch_time = t;
}

double
SimElement::GetHoldTime()
{
	return hold_time;
}

void
SimElement::SetHoldTime(double t)
{
	if (t >= 0)
		hold_time = t;
}

bool
SimElement::GetZoneLock()
{
	return zone_lock;
}

void
SimElement::SetZoneLock(bool z)
{
	zone_lock = z;
}

void
SimElement::SetLoadout(int* l)
{
	if (l) {
		CopyMemory(load, l, sizeof(load));
	}
	else {
		for (int i = 0; i < 16; i++)
			load[i] = -1;
	}
}

// +----------------------------------------------------------------------+

bool
SimElement::Update(SimObject* obj)
{
	// false alarm, keep watching:
	if (obj && obj->Life() != 0) {
		UE_LOG(LOG_SIM, Warning, TEXT("SimElement (%hs) false update on (%hs) life = %f"),
			Name().data(),
			obj->Name(),
			obj->Life());
		return false;
	}

	Ship* s = (Ship*)obj;
	ships.remove(s);

	if (ships.isEmpty())
		respawns = s->RespawnCount();

	return SimObserver::Update(obj);
}

const char*
SimElement::GetObserverName() const
{
	// Preserve original API (const char*) without pointer truncation:
	static char Buf[128];
	std::snprintf(Buf, sizeof(Buf), "SimElement %s", Name().data());
	return Buf;
}

// +----------------------------------------------------------------------+

void
SimElement::AddNavPoint(Instruction* pt, Instruction* afterPoint, bool send)
{
	if (pt && !flight_plan.contains(pt)) {
		int index = -1;

		if (afterPoint) {
			index = flight_plan.index(afterPoint);

			if (index > -1)
				flight_plan.insert(pt, index + 1);
			else
				flight_plan.append(pt);
		}
		else {
			flight_plan.append(pt);
		}
	}
}

void
SimElement::DelNavPoint(Instruction* pt, bool send)
{
	// XXX MEMORY LEAK
	// This is a small memory leak, but I'm notxsure if it is
	// safe to delete the navpoint when removing it from the
	// flight plan.  Other ships in the element might have
	// pointers to the object...?

	if (pt) {
		int index = flight_plan.index(pt);
		flight_plan.remove(pt);

		//if (send) {
		//	NetUtil::SendNavDelete(this, index);
		//}
	}
}

// +----------------------------------------------------------------------+

void
SimElement::ClearFlightPlan(bool send)
{
	hold_time = 0;
	flight_plan.destroy();
	objectives.destroy();
	instructions.destroy();

	//if (send) {
	//	NetUtil::SendNavDelete(this, -1);
	//}
}

// +----------------------------------------------------------------------+

Instruction*
SimElement::GetNextNavPoint()
{
	if (hold_time <= 0 && flight_plan.size() > 0) {
		ListIter<Instruction> iter = flight_plan;
		while (++iter) {
			Instruction* navpt = iter.value();

			if (navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt->HoldTime() > 0)
				return navpt;

			if (navpt->GetStatus() <= INSTRUCTION_STATUS::ACTIVE)
				return navpt;
		}
	}

	return 0;
}

// +----------------------------------------------------------------------+

int
SimElement::GetNavIndex(const Instruction* n)
{
	int index = 0;

	if (flight_plan.size() > 0) {
		ListIter<Instruction> navpt = flight_plan;
		while (++navpt) {
			index++;
			if (navpt.value() == n)
				return index;
		}
	}

	return 0;
}

// +----------------------------------------------------------------------+

List<Instruction>&
SimElement::GetFlightPlan()
{
	return flight_plan;
}

int
SimElement::FlightPlanLength()
{
	return flight_plan.size();
}

// +----------------------------------------------------------------------+

void
SimElement::ClearObjectives()
{
	objectives.destroy();
}

void
SimElement::AddObjective(Instruction* obj)
{
	objectives.append(obj);
}

Instruction*
SimElement::GetObjective(int index)
{
	if (objectives.isEmpty())
		return 0;

	if (index < 0)
		index = 0;
	else if (index >= objectives.size())
		index = index % objectives.size();

	return objectives.at(index);
}

Instruction*
SimElement::GetTargetObjective()
{
	for (int i = 0; i < objectives.size(); i++) {
		Instruction* obj = objectives[i];

		if (obj->GetStatus() <= INSTRUCTION_STATUS::ACTIVE) {
			switch (obj->GetAction()) {
			case INSTRUCTION_ACTION::INTERCEPT:
			case INSTRUCTION_ACTION::STRIKE:
			case INSTRUCTION_ACTION::ASSAULT:
			case INSTRUCTION_ACTION::SWEEP:
			case INSTRUCTION_ACTION::PATROL:
			case INSTRUCTION_ACTION::RECON:
			case INSTRUCTION_ACTION::ESCORT:
			case INSTRUCTION_ACTION::DEFEND:
				return obj;

			default:
				break;
			}
		}
	}

	return 0;
}

// +----------------------------------------------------------------------+

void
SimElement::ClearInstructions()
{
	instructions.clear();
}

void
SimElement::AddInstruction(const char* instr)
{
	instructions.append(new  Text(instr));
}

Text
SimElement::GetInstruction(int index)
{
	if (instructions.isEmpty())
		return Text();

	if (index < 0)
		index = 0;

	if (index >= instructions.size())
		index = index % instructions.size();

	return *instructions.at(index);
}

// +----------------------------------------------------------------------+

void
SimElement::ResumeAssignment()
{
	SetAssignment(0);

	if (objectives.isEmpty())
		return;

	Instruction* objective = 0;

	for (int i = 0; i < objectives.size() && !objective; i++) {
		Instruction* instr = objectives[i];

		if (instr->GetStatus() <= INSTRUCTION_STATUS::ACTIVE) {
			switch (instr->GetAction()) {
			case INSTRUCTION_ACTION::INTERCEPT:
			case INSTRUCTION_ACTION::STRIKE:
			case INSTRUCTION_ACTION::ASSAULT:
				objective = instr;
				break;
			}
		}
	}

	if (objective) {
		Sim* sim = Sim::GetSim();

		ListIter<SimElement> iter = sim->GetElements();
		while (++iter) {
			SimElement* elem = iter.value();
			SimObject* tgt = objective->GetTarget();

			if (tgt && tgt->Type() == SimObject::SIM_SHIP && elem->Contains((const Ship*)tgt)) {
				SetAssignment(elem);
				return;
			}
		}
	}
}

// +----------------------------------------------------------------------+

void
SimElement::HandleRadioMessage(RadioMessage* msg)
{
	if (!msg) return;

	static RadioHandler rh;

	// if this is a message from within the element,
	// then all ships should report in.  Otherwise,
	// just the leader will acknowledge the message.
	int full_report = ships.contains(msg->GetSender());
	int reported = false;

	ListIter<Ship> s = ships;
	while (++s) {
		if (rh.ProcessMessage(msg, s.value())) {
			if (full_report) {
				if (s.value() != msg->GetSender())
					rh.AcknowledgeMessage(msg, s.value());
			}
			else if (!reported) {
				rh.AcknowledgeMessage(msg, s.value());
				reported = true;
			}
		}
	}
}

// +----------------------------------------------------------------------+

bool
SimElement::CanCommand(SimElement* e)
{
	while (e) {
		if (e->commander == this)
			return true;
		e = e->commander;
	}

	return false;
}

// +----------------------------------------------------------------------+

void
SimElement::ExecFrame(double seconds)
{
	if (hold_time > 0) {
		hold_time -= seconds;
		return;
	}

	ListIter<Instruction> iter = flight_plan;
	while (++iter) {
		Instruction* instr = iter.value();

		if (instr->GetStatus() == INSTRUCTION_STATUS::COMPLETE && instr->HoldTime() > 0)
			instr->SetHoldTime(instr->HoldTime() - seconds);
	}
}

// +----------------------------------------------------------------------+

void
SimElement::SetIFF(int NewIFF)
{
	for (int i = 0; i < ships.size(); i++)
		ships[i]->SetIFF(NewIFF);
}
