/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         TacticalAI.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Generic Ship Tactical Level AI class
*/

#include "TacticalAI.h"

#include "ShipAI.h"
#include "CarrierAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "SimContact.h"
#include "WeaponGroup.h"
#include "Drive.h"
#include "Hangar.h"
#include "Sim.h"
#include "SimShot.h"
#include "Drone.h"
#include "StarSystem.h"

#include "Game.h"
#include "Random.h"

#include "Math/UnrealMathUtility.h"
// +--------------------------------------------------------------------+

static int exec_time_seed = 0;

// +--------------------------------------------------------------------+

TacticalAI::TacticalAI(ShipAI* ai)
	: ship(0)
	, ship_ai(0)
	, carrier_ai(0)
	, navpt(0)
	, orders(0)
	, action(RadioMessageAction::NONE)
	, threat_level(0)
	, support_level(1)
	, directed_tgtid(0)
{
	if (ai) {
		ship_ai = ai;
		ship = ai->GetShip();

		Sim* sim = Sim::GetSim();

		if (ship && ship->GetHangar() && ship->GetCommandAILevel() > 0 &&
			ship != sim->GetPlayerShip()) {
			carrier_ai = new CarrierAI(ship, ship_ai->GetAILevel());
		}
	}

	agression = 0;
	roe = FLEXIBLE;
	element_index = 1;
	exec_time = exec_time_seed;
	exec_time_seed += 17;
}

TacticalAI::~TacticalAI()
{
	delete carrier_ai;
}

// +--------------------------------------------------------------------+

void TacticalAI::ExecFrame(double secs)
{
	const int exec_period = 1000;

	if (!ship || !ship_ai)
		return;

	navpt = ship->GetNextNavPoint();
	orders = ship->GetRadioOrders();

	if ((int)Game::GameTime() - exec_time > exec_period) {
		element_index = ship->GetElementIndex();

		CheckOrders();
		SelectTarget();
		FindThreat();
		FindSupport();

		if (element_index > 1) {
			INSTRUCTION_FORMATION formation = INSTRUCTION_FORMATION::DIAMOND;

			if (orders && orders->GetFormation() >= INSTRUCTION_FORMATION::DIAMOND)
				formation = orders->GetFormation();
			else if (navpt)
				formation = navpt->GetFormation();

			FindFormationSlot(formation);
		}

		ship_ai->SetNavPoint(navpt);

		if (carrier_ai)
			carrier_ai->ExecFrame(secs);

		exec_time += exec_period;
	}
}

// +--------------------------------------------------------------------+

void TacticalAI::CheckOrders()
{
	directed_tgtid = 0;

	if (CheckShipOrders())
		return;

	if (CheckFlightPlan())
		return;

	if (CheckObjectives())
		return;
}

// +--------------------------------------------------------------------+

bool TacticalAI::CheckShipOrders()
{
	return ProcessOrders();
}

// +--------------------------------------------------------------------+

bool TacticalAI::CheckObjectives()
{
	bool        processed = false;
	Ship* ward = nullptr;
	SimElement* elem = ship ? ship->GetElement() : nullptr;

	if (elem) {
		Instruction* obj = elem->GetTargetObjective();

		if (obj) {
			ship_ai->ClearPatrol();

			const INSTRUCTION_ACTION Action = obj->GetAction();

			if (Action != INSTRUCTION_ACTION::NONE) {
				switch (Action) {
				case INSTRUCTION_ACTION::INTERCEPT:
				case INSTRUCTION_ACTION::STRIKE:
				case INSTRUCTION_ACTION::ASSAULT:
				{
					SimObject* tgt = obj->GetTarget();
					if (tgt && tgt->Type() == SimObject::SIM_SHIP) {
						roe = DIRECTED;
						SelectTargetDirected(static_cast<Ship*>(tgt));
					}
				}
				break;

				case INSTRUCTION_ACTION::DEFEND:
				case INSTRUCTION_ACTION::ESCORT:
				{
					SimObject* tgt = obj->GetTarget();
					if (tgt && tgt->Type() == SimObject::SIM_SHIP) {
						roe = DEFENSIVE;
						ward = static_cast<Ship*>(tgt);
					}
				}
				break;

				default:
					break;
				}
			}

			orders = obj;
			processed = true;
		}
	}

	ship_ai->SetWard(ward);
	return processed;
}

// +--------------------------------------------------------------------+
bool TacticalAI::ProcessOrders()
{
	if (ship_ai)
		ship_ai->ClearPatrol();

	if (orders && orders->EMCON() > 0) {
		int desired_emcon = orders->EMCON();

		if (ship_ai && (ship_ai->GetThreat() || ship_ai->GetThreatMissile()))
			desired_emcon = 3;

		if (ship->GetEMCON() != desired_emcon)
			ship->SetEMCON(desired_emcon);
	}

	// FIX: enum class cannot be used as a boolean
	if (orders && orders->GetRadioAction() != RadioMessageAction::NONE) {

		switch (orders->GetRadioAction()) {
		case RadioMessageAction::ATTACK:
		case RadioMessageAction::BRACKET:
		case RadioMessageAction::IDENTIFY:
		{
			bool       tgt_ok = false;
			SimObject* tgt = orders->GetTarget();

			if (tgt && tgt->Type() == SimObject::SIM_SHIP) {
				Ship* tgt_ship = (Ship*)tgt;

				if (CanTarget(tgt_ship)) {
					roe = DIRECTED;
					SelectTargetDirected((Ship*)tgt);

					ship_ai->SetBracket(orders->GetRadioAction() == RadioMessageAction::BRACKET);
					ship_ai->SetIdentify(orders->GetRadioAction() == RadioMessageAction::IDENTIFY);
					ship_ai->SetNavPoint(0);

					tgt_ok = true;
				}
			}

			if (!tgt_ok)
				ClearRadioOrders();
		}
		break;

		case RadioMessageAction::ESCORT:
		case RadioMessageAction::COVER_ME:
		{
			SimObject* tgt = orders->GetTarget();
			if (tgt && tgt->Type() == SimObject::SIM_SHIP) {
				roe = DEFENSIVE;
				ship_ai->SetWard((Ship*)tgt);
				ship_ai->SetNavPoint(0);
			}
			else {
				ClearRadioOrders();
			}
		}
		break;

		case RadioMessageAction::WEP_FREE:
			roe = AGRESSIVE;
			ship_ai->DropTarget(0.1);
			break;

		case RadioMessageAction::WEP_HOLD:
		case RadioMessageAction::FORM_UP:
			roe = NONE;
			ship_ai->DropTarget(5);
			break;

		case RadioMessageAction::MOVE_PATROL:
			roe = SELF_DEFENSIVE;
			ship_ai->SetPatrol(orders->Location());
			ship_ai->SetNavPoint(0);
			ship_ai->DropTarget(FMath::FRandRange(5.0, 10.0));
			break;

		case RadioMessageAction::RTB:
		case RadioMessageAction::DOCK_WITH:
		{
			roe = NONE;

			ship_ai->DropTarget(10);

			if (!ship->GetInbound()) {
				RadioMessage* msg = 0;
				Ship* controller = ship->GetController();

				if (orders->GetRadioAction() == RadioMessageAction::DOCK_WITH && orders->GetTarget()) {
					controller = (Ship*)orders->GetTarget();
				}

				if (!controller) {
					SimElement* elem = ship->GetElement();
					if (elem && elem->GetCommander()) {
						SimElement* cmdr = elem->GetCommander();
						controller = cmdr->GetShip(1);
					}
				}

				if (controller && controller->GetHangar() &&
					controller->GetHangar()->CanStow(ship)) {
					SimRegion* self_rgn = ship->GetRegion();
					SimRegion* rtb_rgn = controller->GetRegion();

					if (self_rgn == rtb_rgn) {
						double range = (controller->Location() - ship->Location()).Length();

						if (range < 50e3) {
							msg = new RadioMessage(controller, ship, RadioMessageAction::CALL_INBOUND);
							RadioTraffic::Transmit(msg);
						}
					}
				}
				else {
					ship->ClearRadioOrders();
				}

				ship_ai->SetNavPoint(0);
			}
		}
		break;

		case RadioMessageAction::QUANTUM_TO:
		case RadioMessageAction::FARCAST_TO:
			roe = NONE;
			ship_ai->DropTarget(10);
			break;

		default:
			break;
		}

		action = orders->GetRadioAction();
		return true;
	}

	// if we had an action before, this must be a "cancel orders"
	else if (action != RadioMessageAction::NONE) {
		ClearRadioOrders();
	}

	return false;
}

void TacticalAI::ClearRadioOrders()
{
	action = RadioMessageAction::NONE;
	roe = FLEXIBLE;

	if (ship_ai)
		ship_ai->DropTarget(0.1);

	if (ship)
		ship->ClearRadioOrders();
}

// +--------------------------------------------------------------------+

bool TacticalAI::CheckFlightPlan()
{
	Ship* ward = 0;

	// Find next Instruction:
	navpt = ship->GetNextNavPoint();

	roe = FLEXIBLE;

	if (navpt) {
		switch (navpt->GetAction()) {
		case INSTRUCTION_ACTION::LAUNCH:
		case INSTRUCTION_ACTION::DOCK:
		case INSTRUCTION_ACTION::RTB:
			roe = NONE;
			break;

		case INSTRUCTION_ACTION::VECTOR:
			roe = SELF_DEFENSIVE;
			break;

		case INSTRUCTION_ACTION::DEFEND:
		case INSTRUCTION_ACTION::ESCORT:
			roe = DEFENSIVE;
			break;

		case INSTRUCTION_ACTION::INTERCEPT:
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

		default:
			break;
		}

		if (roe == DEFENSIVE) {
			SimObject* tgt = navpt->GetTarget();

			if (tgt && tgt->Type() == SimObject::SIM_SHIP)
				ward = (Ship*)tgt;
		}

		if (navpt->EMCON() > 0) {
			int desired_emcon = navpt->EMCON();

			if (ship_ai && (ship_ai->GetThreat() || ship_ai->GetThreatMissile()))
				desired_emcon = 3;

			if (ship->GetEMCON() != desired_emcon)
				ship->SetEMCON(desired_emcon);
		}
	}

	if (ship_ai)
		ship_ai->SetWard(ward);

	return (navpt != 0);
}

// +--------------------------------------------------------------------+

void TacticalAI::SelectTarget()
{
	if (!ship) {
		roe = NONE;
		return;
	}

	// unarmed vessels should never engage an enemy:
	if (ship->GetWeapons().size() < 1)
		roe = NONE;

	SimObject* target = ship_ai->GetTarget();
	SimObject* ward = ship_ai->GetWard();

	// if not allowed to engage, drop and return:
	if (roe == NONE) {
		if (target)
			ship_ai->DropTarget();
		return;
	}

	// if we have abandoned our ward, drop and return:
	if (ward && roe != AGRESSIVE) {
		double d = (ward->Location() - ship->Location()).Length();
		double safe_zone = 50e3;

		if (target) {
			if (ship->IsStarship())
				safe_zone = 100e3;

			if (d > safe_zone) {
				ship_ai->DropTarget();
				return;
			}
		}
		else {
			if (d > safe_zone) {
				return;
			}
		}
	}

	// already have a target, keep it:
	if (target) {
		if (target->Life()) {
			CheckTarget();

			// frigates need to be ready to abandon ship-type targets
			// in favor of drone-type targets, others should just go
			// with what they have:
			if (ship->Class() != CLASSIFICATION::CORVETTE && ship->Class() != CLASSIFICATION::FRIGATE)
				return;

			// in case the check decided to drop the target:
			target = ship_ai->GetTarget();
		}
		else {
			ship_ai->DropTarget();
			target = 0;
		}
	}

	// if not allowed to acquire, forget it:
	if (ship_ai->DropTime() > 0)
		return;

	if (roe == DIRECTED) {
		if (target && target->Type() == SimObject::SIM_SHIP)
			SelectTargetDirected((Ship*)target);
		else if (navpt && navpt->GetTarget() && navpt->GetTarget()->Type() == SimObject::SIM_SHIP)
			SelectTargetDirected((Ship*)navpt->GetTarget());
		else
			SelectTargetDirected();
	}
	else {
		SelectTargetOpportunity();

		// don't switch one ship target for another...
		if (ship->Class() == CLASSIFICATION::CORVETTE || ship->Class() == CLASSIFICATION::FRIGATE) {
			SimObject* potential_target = ship_ai->GetTarget();
			if (target && potential_target && target != potential_target) {
				if (target->Type() == SimObject::SIM_SHIP &&
					potential_target->Type() == SimObject::SIM_SHIP) {
					ship_ai->SetTarget(target);
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void TacticalAI::SelectTargetDirected(Ship* tgt)
{
	Ship* potential_target = tgt;

	// try to target one of the element's objectives
	// (if it shows up in the contact list)
	if (!tgt) {
		SimElement* elem = ship->GetElement();

		if (elem) {
			Instruction* objective = elem->GetTargetObjective();

			if (objective) {
				SimObject* obj_sim_obj = objective->GetTarget();
				Ship* obj_tgt = 0;

				if (obj_sim_obj && obj_sim_obj->Type() == SimObject::SIM_SHIP)
					obj_tgt = (Ship*)obj_sim_obj;

				if (obj_tgt) {
					ListIter<SimContact> contact = ship->ContactList();
					while (++contact && !potential_target) {
						Ship* test = contact->GetShip();

						if (obj_tgt == test) {
							potential_target = test;
						}
					}
				}
			}
		}
	}

	if (!CanTarget(potential_target))
		potential_target = 0;

	ship_ai->SetTarget(potential_target);

	if (tgt && tgt == ship_ai->GetTarget())
		directed_tgtid = tgt->Identity();
	else
		directed_tgtid = 0;
}

// +--------------------------------------------------------------------+

bool TacticalAI::CanTarget(Ship* tgt)
{
	bool result = false;

	if (tgt && !tgt->InTransition()) {
		if (tgt->IsRogue() || tgt->GetIFF() != ship->GetIFF())
			result = true;
	}

	return result;
}

// +--------------------------------------------------------------------+

void
TacticalAI::SelectTargetOpportunity()
{
	// NON-COMBATANTS do not pick targets of opportunity:
	if (ship->GetIFF() == 0)
		return;

	SimObject* PotentialTarget = nullptr;

	// pick the closest combatant ship with a different IFF code:
	double TargetDist = ship->Design()->commit_range;

	SimObject* WardObj = ship_ai->GetWard();

	// FRIGATES are primarily anti-air platforms, but may
	// also attack smaller starships:

	if (ship->Class() == CLASSIFICATION::CORVETTE || ship->Class() == CLASSIFICATION::FRIGATE) {
		Ship* CurrentShipTarget = nullptr;
		SimShot* CurrentShotTarget = nullptr;

		// if we are escorting a larger warship, it is good to attack
		// the same target as our ward:

		if (WardObj) {
			Ship* WardShip = (Ship*)WardObj;

			if (WardShip->Class() > ship->Class()) {
				SimObject* WardTarget = WardShip->GetTarget();

				if (WardTarget && WardTarget->Type() == SimObject::SIM_SHIP) {
					CurrentShipTarget = (Ship*)WardTarget;
					TargetDist = (ship->Location() - WardTarget->Location()).Length();
				}
			}
		}

		ListIter<SimContact> ContactIter = ship->ContactList();
		while (++ContactIter) {
			Ship* ContactShip = ContactIter->GetShip();
			SimShot* ContactShot = ContactIter->GetShot();

			if (!ContactShip && !ContactShot)
				continue;

			const int32 ContactIFF = ContactIter->GetIFF(ship);
			const bool bRogue = ContactShip && ContactShip->IsRogue();
			const bool bTargetOk = ContactIFF > 0 &&
				ContactIFF != ship->GetIFF() &&
				ContactIFF < 1000;

			if (bRogue || bTargetOk) {
				if (ContactShip && ContactShip != ship && !ContactShip->InTransition()) {
					if (ContactShip->Class() < CLASSIFICATION::DESTROYER ||
						(ContactShip->Class() >= CLASSIFICATION::MINE && ContactShip->Class() <= CLASSIFICATION::DEFSAT)) {
						// found an enemy, check distance:
						const double Dist = (ship->Location() - ContactShip->Location()).Length();

						if (Dist < 0.75 * TargetDist &&
							(!CurrentShipTarget || ContactShip->Class() <= CurrentShipTarget->Class())) {
							CurrentShipTarget = ContactShip;
							TargetDist = Dist;
						}
					}
				}

				else if (ContactShot) {
					// found an enemy shot, is there enough time to engage?
					if (ContactShot->GetEta() < 3)
						continue;

					// found an enemy shot, check distance:
					const double Dist = (ship->Location() - ContactShot->Location()).Length();

					if (!CurrentShotTarget) {
						CurrentShotTarget = ContactShot;
						TargetDist = Dist;
					}

					// is this shot a better target than the one we've found?
					else {
						// IMPORTANT: SimShot::IsTracking expects Ship*, not SimObject*
						Ship* WardShip = nullptr;
						if (WardObj && WardObj->Type() == SimObject::SIM_SHIP)
							WardShip = (Ship*)WardObj;

						if ((ContactShot->IsTracking(WardShip) || ContactShot->IsTracking(ship)) &&
							(!CurrentShotTarget->IsTracking(WardShip) ||
								!CurrentShotTarget->IsTracking(ship))) {
							CurrentShotTarget = ContactShot;
							TargetDist = Dist;
						}
						else if (Dist < TargetDist) {
							CurrentShotTarget = ContactShot;
							TargetDist = Dist;
						}
					}
				}
			}
		}

		if (CurrentShotTarget)
			PotentialTarget = CurrentShotTarget;
		else
			PotentialTarget = CurrentShipTarget;
	}

	// ALL OTHER SHIP CLASSES ignore fighters and only engage
	// other starships:

	else {
		List<Ship> WardThreats;

		ListIter<SimContact> ContactIter = ship->ContactList();
		while (++ContactIter) {
			Ship* ContactShip = ContactIter->GetShip();

			if (!ContactShip)
				continue;

			const int32 ContactIFF = ContactIter->GetIFF(ship);
			const bool bRogue = ContactShip->IsRogue();
			const bool bTargetOk = ContactShip != ship &&
				ContactIFF > 0 &&
				ContactIFF != ship->GetIFF() &&
				!ContactShip->InTransition();

			if (bRogue || bTargetOk) {
				if (ContactShip->IsStarship() || ContactShip->IsStatic()) {
					// found an enemy, check distance:
					const double Dist = (ship->Location() - ContactShip->Location()).Length();

					if (Dist < 0.75 * TargetDist) {
						PotentialTarget = ContactShip;
						TargetDist = Dist;
					}

					// WardObj is SimObject*, Ship::IsTracking expects SimObject* in your codebase here:
					if (WardObj && ContactShip->IsTracking(WardObj)) {
						WardThreats.append(ContactShip);
					}
				}
			}
		}

		// if this ship is protecting a ward,
		// prefer targets that are threatening that ward:
		if (PotentialTarget && WardThreats.size() && !WardThreats.contains((Ship*)PotentialTarget)) {
			TargetDist *= 2;

			ListIter<Ship> ThreatIter = WardThreats;
			while (++ThreatIter) {
				Ship* ThreatShip = ThreatIter.value();

				const double Dist = (WardObj->Location() - ThreatShip->Location()).Length();

				if (Dist < TargetDist) {
					PotentialTarget = ThreatShip;
					TargetDist = Dist;
				}
			}
		}
	}

	if (ship->Class() != CLASSIFICATION::CARRIER || ship->Class() != CLASSIFICATION::SWACS)
		ship_ai->SetTarget(PotentialTarget);
}


// +--------------------------------------------------------------------+

void TacticalAI::CheckTarget()
{
	SimObject* tgt = ship_ai->GetTarget();

	if (!tgt)
		return;

	if (tgt->GetRegion() != ship->GetRegion()) {
		ship_ai->DropTarget();
		return;
	}

	if (tgt->Type() == SimObject::SIM_SHIP) {
		Ship* target = (Ship*)tgt;

		// has the target joined our side?
		if (target->GetIFF() == ship->GetIFF() && !target->IsRogue()) {
			ship_ai->DropTarget();
			return;
		}

		// is the target already jumping/breaking/dying?
		if (target->InTransition()) {
			ship_ai->DropTarget();
			return;
		}

		// have we been ordered to pursue the target?
		if (directed_tgtid) {
			if (directed_tgtid != target->Identity()) {
				ship_ai->DropTarget();
			}

			return;
		}

		// can we catch the target?
		if (target->Design()->vlimit <= ship->Design()->vlimit ||
			ship->Velocity().Length() <= ship->Design()->vlimit)
			return;

		WeaponDesign* wep_dsn = ship->GetPrimaryDesign();
		if (!wep_dsn)
			return;

		double drop_range = 3 * wep_dsn->max_range;
		if (drop_range > 0.75 * ship->Design()->commit_range)
			drop_range = 0.75 * ship->Design()->commit_range;

		double range = (target->Location() - ship->Location()).Length();
		if (range < drop_range)
			return;

		Point delta = (target->Location() + target->Velocity()) -
			(ship->Location() + ship->Velocity());

		if (delta.Length() < range)
			return;

		ship_ai->DropTarget();
	}
	else if (tgt->Type() == SimObject::SIM_DRONE) {
		Drone* drone = (Drone*)tgt;

		// is the target still a threat?
		if (drone->GetEta() < 1 || drone->GetTarget() == 0)
			ship_ai->DropTarget();
	}
}

// +--------------------------------------------------------------------+

void TacticalAI::FindThreat()
{
	// pick the closest contact on Threat Warning System:
	Ship* threat = 0;
	SimShot* threat_missile = 0;
	Ship* rumor = 0;
	double threat_dist = 1e9;

	const DWORD THREAT_REACTION_TIME = 1000; // 1 second

	ListIter<SimContact> iter = ship->ContactList();

	while (++iter) {
		SimContact* contact = iter.value();

		if (contact->Threat(ship) &&
			(Game::GameTime() - contact->AcquisitionTime()) > THREAT_REACTION_TIME) {

			if (contact->GetShot()) {
				threat_missile = contact->GetShot();
				rumor = (Ship*)threat_missile->Owner();
			}
			else {
				double rng = contact->Range(ship);

				Ship* c_ship = contact->GetShip();
				if (c_ship && !c_ship->InTransition() &&
					c_ship->Class() != CLASSIFICATION::FREIGHTER &&
					c_ship->Class() != CLASSIFICATION::FARCASTER) {

					if (c_ship->GetTarget() == ship) {
						if (!threat || c_ship->Class() > threat->Class()) {
							threat = c_ship;
							threat_dist = 0;
						}
					}
					else if (rng < threat_dist) {
						threat = c_ship;
						threat_dist = rng;
					}
				}
			}
		}
	}

	if (rumor && !rumor->InTransition()) {
		iter.reset();

		while (++iter) {
			if (iter->GetShip() == rumor) {
				rumor = 0;
				ship_ai->ClearRumor();
				break;
			}
		}
	}
	else {
		rumor = 0;
		ship_ai->ClearRumor();
	}

	ship_ai->SetRumor(rumor);
	ship_ai->SetThreat(threat);
	ship_ai->SetThreatMissile(threat_missile);
}

// +--------------------------------------------------------------------+

void TacticalAI::FindSupport()
{
	if (!ship_ai->GetThreat()) {
		ship_ai->SetSupport(0);
		return;
	}

	// pick the biggest friendly contact in the sector:
	Ship* support = 0;
	double support_dist = 1e9;

	ListIter<SimContact> contact = ship->ContactList();

	while (++contact) {
		if (contact->GetShip() && contact->GetIFF(ship) == ship->GetIFF()) {
			Ship* c_ship = contact->GetShip();

			if (c_ship != ship && c_ship->Class() >= ship->Class() && !c_ship->InTransition()) {
				if (!support || c_ship->Class() > support->Class())
					support = c_ship;
			}
		}
	}

	ship_ai->SetSupport(support);
}

// +--------------------------------------------------------------------+

void TacticalAI::FindFormationSlot(INSTRUCTION_FORMATION formation)
{
	// find the formation delta:
	int   s = element_index - 1;
	FVector delta = FVector(10 * s, 0, 10 * s);

	// diamond:
	if (formation == INSTRUCTION_FORMATION::DIAMOND) {
		switch (element_index) {
		case 2: delta = FVector(10, 0, -12); break;
		case 3: delta = FVector(-10, 0, -12); break;
		case 4: delta = FVector(0, 0, -24); break;
		}
	}

	// spread:
	if (formation == INSTRUCTION_FORMATION::SPREAD) {
		switch (element_index) {
		case 2: delta = FVector(15, 0, 0); break;
		case 3: delta = FVector(-15, 0, 0); break;
		case 4: delta = FVector(-30, 0, 0); break;
		}
	}

	// box:
	if (formation == INSTRUCTION_FORMATION::BOX) {
		switch (element_index) {
		case 2: delta = FVector(15, 0, 0); break;
		case 3: delta = FVector(0, -1, -15); break;
		case 4: delta = FVector(15, -1, -15); break;
		}
	}

	// trail:
	if (formation == INSTRUCTION_FORMATION::TRAIL) {
		delta = FVector(0, 0, -15 * s);
	}

	ship_ai->SetFormationDelta(delta * ship->Radius() * 2);
}
