/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Instruction.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Navigation Point class implementation
*/

#include "Instruction.h"

#include "SimElement.h"
#include "RadioMessage.h"
#include "Ship.h"
#include "Sim.h"

#include "Game.h"
#include "Text.h"

// Minimal Unreal includes (used only for logging + FVector):
#include "Math/Vector.h"
#include "Logging/LogMacros.h"
#include "Containers/StringConv.h"
#include "GameStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogTempInstruction, Log, All);

// +----------------------------------------------------------------------+

Instruction::Instruction(EInstruction act, const char* tgt)
	: region(0),
	action(act),
	formation(INSTRUCTION_FORMATION::DIAMOND),
	tgt_name(tgt),
	status(INSTRUCTION_STATUS::PENDING),
	speed(0),
	target(0),
	emcon(0),
	wep_free(0),
	priority(PRIMARY),
	farcast(0),
	hold_time(0)
{
}

Instruction::Instruction(const char* rgn, FVector loc, EInstruction act)
	: region(0),
	action(act),
	formation(INSTRUCTION_FORMATION::DIAMOND),
	status(INSTRUCTION_STATUS::PENDING),
	speed(0),
	target(0),
	emcon(0),
	wep_free(0),
	priority(PRIMARY),
	farcast(0),
	hold_time(0)
{
	rgn_name = rgn;
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(0);
}

Instruction::Instruction(SimRegion* rgn, FVector loc, EInstruction act)
	: region(rgn),
	action(act),
	formation(INSTRUCTION_FORMATION::DIAMOND),
	status(INSTRUCTION_STATUS::PENDING),
	speed(0),
	target(0),
	emcon(0),
	wep_free(0),
	priority(PRIMARY),
	farcast(0),
	hold_time(0)
{
	rgn_name = region->GetName();
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(0);
}

Instruction::Instruction(const Instruction& instr)
	: region(instr.region),
	rgn_name(instr.rgn_name),
	rloc(instr.rloc),
	action(instr.action),
	formation(instr.formation),
	status(instr.status),
	speed(instr.speed),
	target(0),
	tgt_name(instr.tgt_name),
	tgt_desc(instr.tgt_desc),
	emcon(instr.emcon),
	wep_free(instr.wep_free),
	priority(instr.priority),
	farcast(instr.farcast),
	hold_time(instr.hold_time)
{
	SetTarget(instr.target);
}

Instruction::~Instruction()
{
}

// +--------------------------------------------------------------------+

Instruction&
Instruction::operator=(const Instruction& n)
{
	rgn_name = n.rgn_name;
	region = n.region;
	rloc = n.rloc;
	action = n.action;
	formation = n.formation;
	status = n.status;
	speed = n.speed;

	tgt_name = n.tgt_name;
	tgt_desc = n.tgt_desc;
	target = 0;
	emcon = n.emcon;
	wep_free = n.wep_free;
	priority = n.priority;
	farcast = n.farcast;
	hold_time = n.hold_time;

	SetTarget(n.target);
	return *this;
}

// +--------------------------------------------------------------------+

FVector
Instruction::GetLocation() const
{
	Instruction* pThis = (Instruction*)this;
	return pThis->rloc.Location();
}

void
Instruction::SetLocation(const FVector& l)
{
	rloc.SetBaseLocation(l);
	rloc.SetReferenceLoc(0);
	rloc.SetDistance(0);
}

// +----------------------------------------------------------------------+

SimObject*
Instruction::GetTarget()
{
	if (!target && tgt_name.length() > 0) {
		Sim* sim = Sim::GetSim();

		if (sim) {
			Ship* s = sim->FindShip(tgt_name, rgn_name);

			if (s) {
				target = s;
				Observe(target);
			}
		}
	}

	return target;
}

void
Instruction::SetTarget(const FString& InTarget)
{
	SetTarget(TCHAR_TO_ANSI(*InTarget));
}

void
Instruction::SetTarget(const char* InTarget)
{
	const char* SafeTarget =
		InTarget ? InTarget : "";

	if (tgt_name != SafeTarget)
	{
		tgt_name = SafeTarget;
		tgt_desc = SafeTarget;

		// Clear resolved pointer so Sim::ResolveInstructionTargets()
		// can re-resolve it later.
		target = nullptr;
	}
}

void
Instruction::SetTarget(SimObject* s)
{
	if (s && target != s) {
		tgt_name = s->GetName();
		target = s;
		Observe(target);
	}
}

void
Instruction::SetTargetDesc(const char* d)
{
	if (d && *d)
		tgt_desc = d;
}

void
Instruction::ClearTarget()
{
	if (target) {
		target = 0;
		tgt_name = "";
		tgt_desc = "";
	}
}

// +----------------------------------------------------------------------+

bool
Instruction::Update(SimObject* obj)
{
	if (target == obj)
		target = 0;

	return SimObserver::Update(obj);
}

const char*
Instruction::GetObserverName() const
{
	return "Instruction";
}

// +----------------------------------------------------------------------+

void
Instruction::Evaluate(Ship* ship)
{
	Sim* sim = Sim::GetSim();

	switch (action) {
	case EInstruction::Vector:
		break;

	case EInstruction::Launch:
		if (ship->GetFlightPhase() == EOPSMode::ACTIVE)
			SetStatus(INSTRUCTION_STATUS::COMPLETE);
		break;

	case EInstruction::Dock:
	case EInstruction::RTB:
		if (sim->GetPlayerShip() == ship &&
			(ship->GetFlightPhase() == EOPSMode::DOCKING ||
				ship->GetFlightPhase() == EOPSMode::DOCKED))
			SetStatus(INSTRUCTION_STATUS::COMPLETE);
		else if (ship->GetIntegrity() < 1)
			SetStatus(INSTRUCTION_STATUS::FAILED);
		break;

	case EInstruction::Defend:
	case EInstruction::Escort:
	{
		bool found = false;
		bool safe = true;

		ListIter<SimElement> iter = sim->GetElements();
		while (++iter && !found) {
			SimElement* e = iter.value();

			if (e->IsFinished() || e->IsSquadron())
				continue;

			if (e->GetName() == tgt_name ||
				(e->GetCommander() && e->GetCommander()->GetName() == tgt_name)) {

				found = true;

				for (int i = 0; i < e->GetNumShips(); i++) {
					Ship* s = e->GetShip(i + 1);

					if (s && s->GetIntegrity() < 1)
						SetStatus(INSTRUCTION_STATUS::FAILED);
				}

				if (status == INSTRUCTION_STATUS::PENDING) {
					// if the element had a flight plan, and all nav points
					// have been addressed, then the element is safe
					if (e->FlightPlanLength() > 0) {
						if (e->GetNextNavPoint() == 0)
							SetStatus(INSTRUCTION_STATUS::COMPLETE);
						else
							safe = false;
					}
				}
			}
		}

		if (status == INSTRUCTION_STATUS::PENDING && safe &&
			sim->GetPlayerShip() == ship &&
			(ship->GetFlightPhase() == EOPSMode::DOCKING ||
				ship->GetFlightPhase() == EOPSMode::DOCKED)) {
			SetStatus(INSTRUCTION_STATUS::COMPLETE);
		}
	}
	break;

	case EInstruction::Patrol:
	case EInstruction::Sweep:
	{
		bool alive = false;

		ListIter<SimElement> iter = sim->GetElements();
		while (++iter) {
			SimElement* e = iter.value();

			if (e->IsFinished() || e->IsSquadron())
				continue;

			if (e->GetIFF() && e->GetIFF() != ship->GetIFF()) {
				for (int i = 0; i < e->GetNumShips(); i++) {
					Ship* s = e->GetShip(i + 1);

					if (s && s->GetIntegrity() >= 1)
						alive = true;
				}
			}
		}

		if (status == INSTRUCTION_STATUS::PENDING && !alive) {
			SetStatus(INSTRUCTION_STATUS::COMPLETE);
		}
	}
	break;

	case EInstruction::Intercept:
	case EInstruction::Strike:
	case EInstruction::Assault:
	{
		bool alive = false;

		ListIter<SimElement> iter = sim->GetElements();
		while (++iter) {
			SimElement* e = iter.value();

			if (e->IsFinished() || e->IsSquadron())
				continue;

			if (e->GetName() == tgt_name) {
				for (int i = 0; i < e->GetNumShips(); i++) {
					Ship* s = e->GetShip(i + 1);

					if (s && s->GetIntegrity() >= 1)
						alive = true;
				}
			}
		}

		if (status == INSTRUCTION_STATUS::PENDING && !alive) {
			SetStatus(INSTRUCTION_STATUS::COMPLETE);
		}
	}
	break;

	case EInstruction::Recon:
		break;

	default:
		break;
	}
}

void
Instruction::SetStatus(INSTRUCTION_STATUS s)
{
	status = s;
}

// +----------------------------------------------------------------------+

const char*
Instruction::GetShortDescription() const
{
	static char desc[256];

	switch (action) {
	case EInstruction::Vector:
		if (farcast)
			sprintf_s(desc, "Farcast to %s sector", rgn_name.data());
		else
			sprintf_s(desc, "Go to %s sector", rgn_name.data());
		break;

	case EInstruction::Launch:
		sprintf_s(desc, "Launch from the %s", tgt_name.data());
		break;

	case EInstruction::Dock:
		sprintf_s(desc, "Dock with the %s", tgt_name.data());
		break;

	case EInstruction::RTB:
		sprintf_s(desc, "Return safely to base");
		break;

	case EInstruction::Defend:
		if (priority == PRIMARY) {
			sprintf_s(desc, "Defend %s", tgt_desc.data());
		}
		else {
			sprintf_s(desc, "Protect %s in the area", tgt_desc.data());
		}
		break;

	case EInstruction::Escort:
		if (priority == PRIMARY) {
			sprintf_s(desc, "Escort %s", tgt_desc.data());
		}
		else {
			sprintf_s(desc, "Protect %s in the area", tgt_desc.data());
		}
		break;
	case EInstruction::Patrol:
		sprintf_s(desc, "Patrol for %s in %s",
			tgt_desc.data(),
			rgn_name.data());
		break;

	case EInstruction::Sweep:
		sprintf_s(desc, "Sweep for %s in %s",
			tgt_desc.data(),
			rgn_name.data());
		break;

	case EInstruction::Intercept:
		sprintf_s(desc, "Intercept and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Strike:
		sprintf_s(desc, "Engage and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Assault:
		sprintf_s(desc, "Engage and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Recon:
		sprintf_s(desc,"Recon scan %s", tgt_desc.data());
		break;

	default:
		sprintf_s(desc, "%s", ActionName(action));
		break;
	}

	if (status != INSTRUCTION_STATUS::PENDING) {
		strcat_s(desc, " - ");
		strcat_s(desc, StatusName(status));
	}

	return desc;
}

// +----------------------------------------------------------------------+


const char*
Instruction::GetDescription() const
{
	static char desc[1024];

	switch (action) {
	case EInstruction::Vector:
		if (farcast)
			sprintf_s(desc, "Farcast to the %s sector", rgn_name.data());
		else
			sprintf_s(desc, "Go to the %s sector", rgn_name.data());
		break;

	case EInstruction::Launch:
		sprintf_s(desc, "Launch from the %s", tgt_name.data());
		break;

	case EInstruction::Dock:
		sprintf_s(desc, "Dock with the %s", tgt_name.data());
		break;

	case EInstruction::RTB:
		sprintf_s(desc, "Return safely to base");
		break;

	case EInstruction::Defend:
		if (priority == PRIMARY) {
			sprintf_s(desc, "Defend %s", tgt_desc.data());
		}
		else {
			sprintf_s(desc, "Protect %s in the area", tgt_desc.data());
		}
		break;

	case EInstruction::Escort:
		if (priority == PRIMARY) {
			sprintf_s(desc, "Escort %s", tgt_desc.data());
		}
		else {
			sprintf_s(desc, "Protect %s in the area", tgt_desc.data());
		}
		break;

	case EInstruction::Patrol:
		sprintf_s(desc, "Disable or destroy %s in the %s sector",
			tgt_desc.data(),
			rgn_name.data());
		break;

	case EInstruction::Sweep:
		sprintf_s(desc, "Disable or destroy %s in the %s sector",
			tgt_desc.data(),
			rgn_name.data());
		break;

	case EInstruction::Intercept:
		sprintf_s(desc, "Intercept and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Strike:
		sprintf_s(desc, "Engage and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Assault:
		sprintf_s(desc, "Engage and destroy %s", tgt_desc.data());
		break;

	case EInstruction::Recon:
		sprintf_s(desc, "Recon scan %s", tgt_desc.data());
		break;

	default:
		sprintf_s(desc, "%s", ActionName(action));
		break;
	}

	if (status != INSTRUCTION_STATUS::PENDING) {
		strcat_s(desc, " - ");
		strcat_s(desc, StatusName(status));
	}

	return desc;
}

// +----------------------------------------------------------------------+

const char*
Instruction::ActionName(EInstruction a)
{
	switch (a) {
	case EInstruction::Vector: 
		return "Vector";
	case EInstruction::Launch:
		return "Launch";
	case EInstruction::Dock:  
		return "Dock";
	case EInstruction::RTB:  
		return "RTB";
	case EInstruction::Defend: 
		return "Defend";
	case EInstruction::Escort:  
		return "Escort";
	case EInstruction::Patrol:
		return "Patrol";
	case EInstruction::Sweep: 
		return "Sweep";
	case EInstruction::Intercept: 
		return "Intercept";
	case EInstruction::Strike:  
		return "Strike";
	case EInstruction::Assault: 
		return "Assault";
	case EInstruction::Recon: 
		return "Recon";

	default:      
		return "Unknown";
	}
}

const char* Instruction::ActionName(int ActionIndex)
{
	return ActionName(
		static_cast<EInstruction>(ActionIndex)
	);
}

const char*
Instruction::StatusName(INSTRUCTION_STATUS s)
{
	switch (s) {
	case INSTRUCTION_STATUS::PENDING: 
		return "Pending";
	case INSTRUCTION_STATUS::ACTIVE: 
		return "Active";
	case INSTRUCTION_STATUS::SKIPPED: 
		return "Skipped";
	case INSTRUCTION_STATUS::ABORTED:  
		return "Aborted";
	case INSTRUCTION_STATUS::FAILED: 
		return "Failed";
	case INSTRUCTION_STATUS::COMPLETE:
		return "Complete";

	default:       return "Unknown";
	}
}

const char*
Instruction::FormationName(INSTRUCTION_FORMATION f)
{
	switch (f) {
	case INSTRUCTION_FORMATION::DIAMOND: 
		return "Diamond";
	case INSTRUCTION_FORMATION::SPREAD:
		return "Spread";
	case INSTRUCTION_FORMATION::BOX:  
		return "Box";
	case INSTRUCTION_FORMATION::TRAIL:  
		return "Trail";

	default:      return "Unknown";
	}
}

const char*
Instruction::PriorityName(int p)
{
	switch (p) {
	case PRIMARY:   return "Primary";
	case SECONDARY: return "Secondary";
	case BONUS:     return "Bonus";

	default:        return "Unknown";
	}
}
