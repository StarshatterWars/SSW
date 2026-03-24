/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         FighterTacticalAI.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Fighter-specific mid-level (tactical) AI class
*/

#include "FighterTacticalAI.h"

#include "ShipAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimShot.h"
#include "SimElement.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "Sensor.h"
#include "SimContact.h"
#include "WeaponGroup.h"
#include "Drive.h"
#include "Sim.h"
#include "StarSystem.h"
#include "GameStructs.h"

#include "Game.h"

// Minimal Unreal include for UE RNG and UE integer types:
#include "Math/UnrealMathUtility.h"

// +----------------------------------------------------------------------+

static const int WINCHESTER_FIGHTER = 0;
static const int WINCHESTER_ASSAULT = 1;
static const int WINCHESTER_STRIKE = 2;
static const int WINCHESTER_STATIC = 3;

// +----------------------------------------------------------------------+

FighterTacticalAI::FighterTacticalAI(ShipAI* ai)
	: TacticalAI(ai), secondary_selection_time(0)
{
	for (int i = 0; i < 4; i++)
		winchester[i] = false;

	ai_level = ai->GetAILevel();

	switch (ai_level) {
	default:
	case 2:  THREAT_REACTION_TIME = 1000; break;
	case 1:  THREAT_REACTION_TIME = 3000; break;
	case 0:  THREAT_REACTION_TIME = 6000; break;
	}
}


// +--------------------------------------------------------------------+

FighterTacticalAI::~FighterTacticalAI()
{
}

// +--------------------------------------------------------------------+

bool
FighterTacticalAI::CheckFlightPlan()
{
	navpt = ship->GetNextNavPoint();

	INSTRUCTION_ACTION order = INSTRUCTION_ACTION::PATROL;
	roe = FLEXIBLE;

	if (navpt) {
		order = navpt->GetAction();

		switch (order) {
		case INSTRUCTION_ACTION::LAUNCH:
		case INSTRUCTION_ACTION::DOCK:
		case INSTRUCTION_ACTION::RTB:     
			roe = NONE;
			break;

		case INSTRUCTION_ACTION::VECTOR:
			roe = SELF_DEFENSIVE;
			if (element_index > 1)
				roe = DEFENSIVE;
			break;

		case INSTRUCTION_ACTION::DEFEND:
		case INSTRUCTION_ACTION::ESCORT: 
			roe = DEFENSIVE;
			break;

		case INSTRUCTION_ACTION::INTERCEPT:
			if (element_index > 1)
				roe = DEFENSIVE;
			else
				roe = DIRECTED;
			break;

		case INSTRUCTION_ACTION::RECON:
		case INSTRUCTION_ACTION::STRIKE:
		case INSTRUCTION_ACTION::ASSAULT: 
			roe = DIRECTED;
			break;

		case INSTRUCTION_ACTION::PATROL:
		case INSTRUCTION_ACTION::SWEEP:   
			roe = FLEXIBLE;
			break;

		default: break;
		}

		if (order == INSTRUCTION_ACTION::STRIKE) {
			ship->SetSensorMode(Sensor::GM);

			if (IsStrikeComplete(navpt)) {
				ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
			}
		}

		else if (order == INSTRUCTION_ACTION::ASSAULT) {
			if (ship->GetSensorMode() == Sensor::GM)
				ship->SetSensorMode(Sensor::STD);

			if (IsStrikeComplete(navpt)) {
				ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
			}
		}

		else {
			if (ship->GetSensorMode() == Sensor::GM)
				ship->SetSensorMode(Sensor::STD);
		}
	}

	switch (roe) {
	case NONE:              ship->SetDirectorInfo("Cruise");
		break;
	case SELF_DEFENSIVE:    ship->SetDirectorInfo("SELF DEF"); 
		break;
	case DEFENSIVE:         ship->SetDirectorInfo("DEFENSE");
		break;
	case DIRECTED:          ship->SetDirectorInfo("DIRECT");
		break;
	case FLEXIBLE:          ship->SetDirectorInfo("FLEX");
		break;
	default:                ship->SetDirectorInfo("???");
		break;
	}

	return (navpt != 0);
}

// +--------------------------------------------------------------------+

void
FighterTacticalAI::SelectTarget()
{
	TacticalAI::SelectTarget();

	SimObject* target = ship_ai->GetTarget();

	if (target && (target->Type() == SimObject::SIM_SHIP) &&
		(Game::GameTime() - secondary_selection_time) > THREAT_REACTION_TIME) {
		SelectSecondaryForTarget((Ship*)target);
		secondary_selection_time = Game::GameTime();
	}
}

// +--------------------------------------------------------------------+

void
FighterTacticalAI::SelectTargetDirected(Ship* tgt)
{
	Ship* potential_target = tgt;

	if (!tgt) {
		// try to target one of the element's objectives
		// (if it shows up in the contact list)

		SimElement* elem = ship->GetElement();

		if (elem) {
			Instruction* objective = elem->GetTargetObjective();

			if (objective) {
				SimObject* obj_sim_obj = objective->GetTarget();
				Ship* obj_tgt = 0;

				if (obj_sim_obj && obj_sim_obj->Type() == SimObject::SIM_SHIP)
					obj_tgt = (Ship*)obj_sim_obj;

				if (obj_tgt && ship->FindContact(obj_tgt))
					potential_target = obj_tgt;
			}
		}
	}

	if (!CanTarget(potential_target))
		potential_target = 0;

	ship_ai->SetTarget(potential_target);
	SelectSecondaryForTarget(potential_target);
}

// +--------------------------------------------------------------------+

void FighterTacticalAI::SelectTargetOpportunity()
{
	// NON-COMBATANTS do not pick targets of opportunity:
	if (!ship || ship->GetIFF() == 0)
		return;

	Ship* PotentialTarget = nullptr;
	SimShot* CurrentShotTarget = nullptr;

	// pick the closest combatant ship with a different IFF code:
	double TargetDist = 1.0e15;
	const double MinDist = 5.0e3; // kept for parity (not used in original selection logic)

	// FIGHTERS are primarily anti-air platforms, but may also attack smaller starships:
	Ship* Ward = nullptr;
	if (element_index > 1)
		Ward = ship->GetLeader();

	// commit range for patrol/sweep is 80 Km
	// (about 2 minutes for a fighter at max speed)
	if (roe == FLEXIBLE || roe == AGRESSIVE)
		TargetDist = ship->Design()->commit_range;

	if (roe < FLEXIBLE)
		TargetDist = 0.5 * ship->Design()->commit_range;

	int ClassLimit = (int)CLASSIFICATION::LCA;
	if (ship->Class() == CLASSIFICATION::ATTACK)
		ClassLimit = (int)CLASSIFICATION::DESTROYER;

	ListIter<SimContact> ContactIter = ship->ContactList();
	while (++ContactIter) {
		SimContact* Contact = ContactIter.value();
		if (!Contact)
			continue;

		Ship* ContactShip = Contact->GetShip();
		SimShot* ContactShot = Contact->GetShot();
		const int ContactIFF = Contact->GetIFF(ship);

		bool bRogue = false;
		if (ContactShip)
			bRogue = ContactShip->IsRogue();

		if (!bRogue && (ContactIFF <= 0 || ContactIFF == ship->GetIFF() || ContactIFF == 1000))
			continue;

		// reasonable target?
		if (ContactShip && (int) ContactShip->Class() <= ClassLimit && !ContactShip->InTransition()) {

			if (!bRogue) {
				SimObject* TheirTarget = ContactShip->GetTarget();

				// if we are self-defensive, is this contact engaging us?
				if (roe == SELF_DEFENSIVE && TheirTarget != ship)
					continue;

				// if we are defending, is this contact engaging us or our ward?
				if (roe == DEFENSIVE && TheirTarget != ship && TheirTarget != Ward)
					continue;
			}

			// found an enemy, check distance:
			const double Dist = (ship->Location() - ContactShip->Location()).Length();

			if (Dist < 0.75 * TargetDist) {

				// if on patrol, check target distance from navpoint:
				if (roe == FLEXIBLE && navpt) {
					// UE: FVector has no OtherHand(). Use the navpoint location directly.
					// If you still need Starshatter axis remapping, do it explicitly here.
					const double NDist = (navpt->Location() - ContactShip->Location()).Length();
					if (NDist > 80e3)
						continue;
				}

				PotentialTarget = ContactShip;
				TargetDist = Dist;
			}
		}

		else if (ContactShot && ContactShot->IsDrone()) {
			// found an enemy shot, do we have enough time to engage?
			if (ContactShot->GetEta() < 10)
				continue;

			// found an enemy shot, check distance:
			const double Dist = (ship->Location() - ContactShot->Location()).Length();

			if (!CurrentShotTarget) {
				CurrentShotTarget = ContactShot;
				TargetDist = Dist;
			}
			else {
				Ship* WardX = ship_ai ? ship_ai->GetWard() : nullptr;

				if ((WardX && ContactShot->IsTracking(WardX) && !CurrentShotTarget->IsTracking(WardX)) ||
					(Dist < TargetDist)) {
					CurrentShotTarget = ContactShot;
					TargetDist = Dist;
				}
			}
		}
	}

	if (ship_ai) {
		if (CurrentShotTarget) {
			ship_ai->SetTarget(CurrentShotTarget);
		}
		else {
			ship_ai->SetTarget(PotentialTarget);
			SelectSecondaryForTarget(PotentialTarget);
		}
	}
}

// +--------------------------------------------------------------------+

int
FighterTacticalAI::ListSecondariesForTarget(Ship* tgt, List<WeaponGroup>& weps)
{
	weps.clear();

	if (tgt) {
		ListIter<WeaponGroup> iter = ship->GetWeapons();
		while (++iter) {
			WeaponGroup* w = iter.value();

			if (w->Ammo() && w->CanTarget((uint32) tgt->Class()))
				weps.append(w);
		}
	}

	return weps.size();
}

void
FighterTacticalAI::SelectSecondaryForTarget(Ship* tgt)
{
	if (tgt) {
		int wix = WINCHESTER_FIGHTER;

		if (tgt->IsGroundUnit())      wix = WINCHESTER_STRIKE;
		else if (tgt->IsStatic())     wix = WINCHESTER_STATIC;
		else if (tgt->IsStarship())   wix = WINCHESTER_ASSAULT;

		WeaponGroup* best = 0;
		List<WeaponGroup> weps;

		if (ListSecondariesForTarget(tgt, weps)) {
			winchester[wix] = false;

			// select best weapon for the job:
			double range = (ship->Location() - tgt->Location()).Length();
			double best_range = 0;
			double best_damage = 0;

			ListIter<WeaponGroup> iter = weps;
			while (++iter) {
				WeaponGroup* w = iter.value();

				if (!best) {
					best = w;

					WeaponDesign* d = best->GetDesign();
					best_range = d->max_range;
					best_damage = d->damage * d->ripple_count;

					if (best_range < range)
						best = 0;
				}

				else {
					WeaponDesign* d = w->GetDesign();
					double w_range = d->max_range;
					double w_damage = d->damage * d->ripple_count;

					if (w_range > range) {
						if (w_range < best_range || w_damage > best_damage)
							best = w;
					}
				}
			}

			// now cycle weapons until you pick the best one:
			WeaponGroup* current_missile = ship->GetSecondaryGroup();

			if (current_missile && best && current_missile != best) {
				ship->CycleSecondary();
				WeaponGroup* m = ship->GetSecondaryGroup();

				while (m != current_missile && m != best) {
					ship->CycleSecondary();
					m = ship->GetSecondaryGroup();
				}
			}
		}

		else {
			winchester[wix] = true;

			// if we have NO weapons that can hit this target,
			// just drop it:

			Weapon* primary = ship->GetPrimary();
			if (!primary || !primary->CanTarget((uint32)tgt->Class())) {
				ship_ai->DropTarget(3);
				ship->DropTarget();
			}
		}

		if (tgt->IsGroundUnit())
			ship->SetSensorMode(Sensor::GM);

		else if (ship->GetSensorMode() == Sensor::GM)
			ship->SetSensorMode(Sensor::STD);
	}
}

// +--------------------------------------------------------------------+

void
FighterTacticalAI::FindFormationSlot(INSTRUCTION_FORMATION formation)
{
	// find the formation delta:
	int s = element_index - 1;
	Point delta(5 * s, 0, -5 * s);

	// diamond:
	if (formation == INSTRUCTION_FORMATION::DIAMOND) {
		switch (element_index) {
		case 2:  delta = FVector(12, -1, -10); break;
		case 3:  delta = FVector(-12, -1, -10); break;
		case 4:  delta = FVector(0, -2, -20); break;
		}
	}

	// spread:
	if (formation == INSTRUCTION_FORMATION::SPREAD) {
		switch (element_index) {
		case 2:  delta = FVector(15, 0, 0); break;
		case 3:  delta = FVector(-15, 0, 0); break;
		case 4:  delta = FVector(-30, 0, 0); break;
		}
	}

	// box:
	if (formation == INSTRUCTION_FORMATION::BOX) {
		switch (element_index) {
		case 2:  delta = FVector(15, 0, 0); break;
		case 3:  delta = FVector(0, -2, -20); break;
		case 4:  delta = FVector(15, -2, -20); break;
		}
	}

	// trail:
	if (formation == INSTRUCTION_FORMATION::TRAIL) {
		delta = FVector(0, s, -20 * s);
	}

	ship_ai->SetFormationDelta(delta * ship->Radius() * 2);
}

// +--------------------------------------------------------------------+

void
FighterTacticalAI::FindThreat()
{
	// pick the closest contact on Threat Warning System:
	Ship* threat_ship = 0;
	SimShot* threat_missile = 0;
	double      threat_dist = 1e9;

	ListIter<SimContact> c_iter = ship->ContactList();

	while (++c_iter) {
		SimContact* contact = c_iter.value();

		if (contact->Threat(ship) &&
			(Game::GameTime() - contact->AcquisitionTime()) > THREAT_REACTION_TIME) {

			double rng = contact->Range(ship);

			if (contact->GetShot()) {
				threat_missile = contact->GetShot();
			}

			else if (rng < threat_dist && contact->GetShip()) {
				Ship* candidate = contact->GetShip();

				if (candidate->InTransition())
					continue;

				if (candidate->IsStarship() && rng < 50e3) {
					threat_ship = candidate;
					threat_dist = rng;
				}

				else if (candidate->IsDropship() && rng < 25e3) {
					threat_ship = candidate;
					threat_dist = rng;
				}

				// static and ground units:
				else if (rng < 30e3) {
					threat_ship = candidate;
					threat_dist = rng;
				}
			}
		}
	}

	ship_ai->SetThreat(threat_ship);
	ship_ai->SetThreatMissile(threat_missile);
}

// +--------------------------------------------------------------------+

bool
FighterTacticalAI::IsStrikeComplete(Instruction* instr)
{
	// wingmen can not call a halt to a strike:
	if (!ship || element_index > 1)
		return false;

	// if there's nothing to shoot at, we must be done:
	if (!instr || !instr->GetTarget() || instr->GetTarget()->Life() == 0 ||
		instr->GetTarget()->Type() != SimObject::SIM_SHIP)
		return true;

	// break off strike only when ALL weapons are expended:
	// (remember to check all relevant wingmen)
	SimElement* element = ship->GetElement();
	Ship* target = (Ship*)instr->GetTarget();

	if (!element)
		return true;

	for (int i = 0; i < element->NumShips(); i++) {
		Ship* s = element->GetShip(i + 1);

		if (!s || s->Integrity() < 25) // || (s->Location() - target->Location()).length() > 250e3)
			continue;

		ListIter<WeaponGroup> g_iter = s->GetWeapons();
		while (++g_iter) {
			WeaponGroup* w = g_iter.value();

			if (w->Ammo() && w->CanTarget((uint32)target->Class())) {
				ListIter<Weapon> w_iter = w->GetWeapons();

				while (++w_iter) {
					Weapon* weapon = w_iter.value();

					if (weapon->GetStatus() > SYSTEM_STATUS::CRITICAL)
						return false;
				}
			}
		}
	}

	// still here?  we must be done!
	return true;
}
