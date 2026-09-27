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

void
TacticalAI::ExecFrame(double secs)
{
	const int exec_period = 1000;

	if (!ship || !ship_ai)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::ExecFrame] SKIP Ship=%p ShipAI=%p"),
			ship,
			ship_ai);

		return;
	}

	navpt = ship->GetNextNavPoint();
	orders = ship->GetRadioOrders();

	const int32 Now =
		(int32)Game::GetGameTime();

	const bool bForceTacticalEveryFrameForMigration = true;

	if (bForceTacticalEveryFrameForMigration ||
		Now - exec_time > exec_period)
	{
		exec_time = Now;

		element_index = ship->GetElementIndex();

		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::ExecFrame] RUN Ship='%hs' Time=%d ExecTime=%d ElementIndex=%d Navpt=%p Orders=%p Contacts=%d CurrentTarget='%hs' ROE=%d"),
			ship->GetName(),
			Now,
			exec_time,
			element_index,
			navpt,
			orders,
			ship->GetContactList().size(),
			ship_ai->GetTarget() ? ship_ai->GetTarget()->GetName() : "NULL",
			(int)roe);

		CheckOrders();

		if (roe == NONE &&
			ship->GetContactList().size() > 0)
		{
			roe = AGRESSIVE;

			UE_LOG(LogTemp, Warning,
				TEXT("[TacticalAI::ExecFrame] MIGRATION FORCE ROE Ship='%hs' Contacts=%d"),
				ship->GetName(),
				ship->GetContactList().size());
		}

		SelectTarget();
		FindThreat();
		FindSupport();

		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::ExecFrame] AFTER Ship='%hs' Target='%hs' Threat='%hs' Support='%hs' ThreatLevel=%.4f SupportLevel=%.4f ShipTarget='%hs'"),
			ship->GetName(),
			ship_ai->GetTarget() ? ship_ai->GetTarget()->GetName() : "NULL",
			ship_ai->GetThreat() ? ship_ai->GetThreat()->GetName() : "NULL",
			ship_ai->GetSupport() ? ship_ai->GetSupport()->GetName() : "NULL",
			(double)ThreatLevel(),
			(double)SupportLevel(),
			ship->GetTarget() ? ship->GetTarget()->GetName() : "NULL");

		if (element_index > 1)
		{
			INSTRUCTION_FORMATION formation =
				INSTRUCTION_FORMATION::DIAMOND;

			if (orders &&
				orders->GetFormation() >= INSTRUCTION_FORMATION::DIAMOND)
			{
				formation = orders->GetFormation();
			}
			else if (navpt)
			{
				formation = navpt->GetFormation();
			}

			FindFormationSlot(formation);
		}

		ship_ai->SetNavPoint(navpt);

		if (carrier_ai)
		{
			carrier_ai->ExecFrame(secs);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::ExecFrame] SKIP PERIOD Ship='%hs' Time=%d ExecTime=%d Delta=%d Period=%d Contacts=%d ShipTarget='%hs'"),
			ship->GetName(),
			Now,
			exec_time,
			Now - exec_time,
			exec_period,
			ship->GetContactList().size(),
			ship->GetTarget() ? ship->GetTarget()->GetName() : "NULL");
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[TacticalAI::ExecFrame] Ship='%hs' Contacts=%d ShipTarget='%hs' AITarget='%hs'"),
		ship ? ship->GetName() : "NULL",
		ship ? ship->GetContactList().size() : -1,
		ship && ship->GetTarget() ? ship->GetTarget()->GetName() : "NULL",
		ship_ai && ship_ai->GetTarget() ? ship_ai->GetTarget()->GetName() : "NULL");
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
	bool processed = false;
	Ship* ward = nullptr;
	SimElement* elem = ship ? ship->GetElement() : nullptr;

	if (elem)
	{
		Instruction* obj = elem->GetTargetObjective();

		if (obj)
		{
			ship_ai->ClearPatrol();

			const EInstruction Action = obj->GetAction();

			if (Action != EInstruction::None)
			{
				switch (Action)
				{
				case EInstruction::Intercept:
				case EInstruction::Strike:
				case EInstruction::Assault:
				{
					SimObject* tgt = obj->GetTarget();

					if (tgt && tgt->GetType() == ESimObject::SHIP)
					{
						roe = DIRECTED;
						SelectTargetDirected(static_cast<Ship*>(tgt));
					}
				}
				break;

				case EInstruction::Defend:
				case EInstruction::Escort:
				{
					SimObject* tgt = obj->GetTarget();

					if (tgt && tgt->GetType() == ESimObject::SHIP)
					{
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

		//---------------------------------------------------------
		// COMMANDER FALLBACK
		//
		// Legacy mission commander means this element is subordinate
		// to another SimElement. If no explicit escort/defend ward
		// was assigned above, use commander ship 1 as the ward.
		//---------------------------------------------------------

		if (!ward && elem->GetCommander())
		{
			SimElement* CommanderElement =
				elem->GetCommander();

			Ship* CommanderShip =
				CommanderElement
				? CommanderElement->GetShip(1)
				: nullptr;

			if (CommanderShip && CommanderShip != ship)
			{
				ward = CommanderShip;

				roe = DEFENSIVE;

				UE_LOG(LogTemp, Error,
					TEXT("[TacticalAI::CheckObjectives COMMANDER WARD] Ship='%hs' Element='%hs' CommanderElement='%hs' Ward='%hs'"),
					ship ? ship->GetName() : "NULL",
					elem ? elem->GetName().data() : "NULL",
					CommanderElement ? CommanderElement->GetName().data() : "NULL",
					ward ? ward->GetName() : "NULL");
			}
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

	if (orders && orders->GetEMCON() > 0) {
		int desired_emcon = orders->GetEMCON();

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

			if (tgt && tgt->GetType() == ESimObject::SHIP) {
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
			if (tgt && tgt->GetType() == ESimObject::SHIP) {
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
			ship_ai->SetPatrol(orders->GetLocation());
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
						double range = (controller->GetLocation() - ship->GetLocation()).Length();

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
		case EInstruction::Launch:
		case EInstruction::Dock:
		case EInstruction::RTB:
			roe = NONE;
			break;

		case EInstruction::Vector:
			roe = SELF_DEFENSIVE;
			break;

		case EInstruction::Defend:
		case EInstruction::Escort:
			roe = DEFENSIVE;
			break;

		case EInstruction::Intercept:
			roe = DIRECTED;
			break;

		case EInstruction::Recon:
		case EInstruction::Strike:
		case EInstruction::Assault:
			roe = DIRECTED;
			break;

		case EInstruction::Patrol:
		case EInstruction::Sweep:
			roe = FLEXIBLE;
			break;

		default:
			break;
		}

		if (roe == DEFENSIVE) {
			SimObject* tgt = navpt->GetTarget();

			if (tgt && tgt->GetType() == ESimObject::SHIP)
				ward = (Ship*)tgt;
		}

		if (navpt->GetEMCON() > 0) {
			int desired_emcon = navpt->GetEMCON();

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

void
TacticalAI::SelectTarget()
{
	if (!ship)
	{
		roe = NONE;
		return;
	}

	/*
	 * TEMP UE MIGRATION NOTE:
	 *
	 * Weapons are not fully initialized yet.
	 * Allow sensors and tactical target selection to operate
	 * even when runtime weapon objects are missing.
	 */

	UE_LOG(LogTemp, Warning,
		TEXT("[TacticalAI::SelectTarget ENTER] Ship='%hs' IFF=%d Weapons=%d ROE=%d CurrentTarget='%hs'"),
		ship ? ship->GetName() : "NULL",
		ship ? ship->GetIFF() : -1,
		ship ? ship->GetWeapons().size() : -1,
		(int)roe,
		ship_ai && ship_ai->GetTarget() ?
		ship_ai->GetTarget()->GetName() : "NULL");

	SimObject* target =
		ship_ai ? ship_ai->GetTarget() : nullptr;

	SimObject* ward =
		ship_ai ? ship_ai->GetWard() : nullptr;

	//-------------------------------------------------------------
	// INSTRUCTION TARGET OVERRIDE
	//-------------------------------------------------------------
	if (navpt && navpt->GetTarget())
	{
		ship_ai->SetTarget(navpt->GetTarget());

		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTarget] INSTRUCTION TARGET OVERRIDE Ship='%hs' Target='%hs'"),
			ship ? ship->GetName() : "NULL",
			navpt->GetTarget() ?
			navpt->GetTarget()->GetName() : "NULL");

		return;
	}

	//-------------------------------------------------------------
	// If not allowed to engage, drop and return
	//-------------------------------------------------------------
	if (roe == NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTarget] ROE NONE Ship='%hs' DroppingTarget='%hs'"),
			ship ? ship->GetName() : "NULL",
			target ? target->GetName() : "NULL");

		if (target)
		{
			ship_ai->DropTarget();
		}

		return;
	}

	//-------------------------------------------------------------
	// Formation leash
	//-------------------------------------------------------------
	if (ward && roe != AGRESSIVE)
	{
		double d =
			(ward->GetLocation() -
				ship->GetLocation()).Size();

		double safe_zone = 50e3;

		if (target)
		{
			if (ship->IsStarship())
			{
				safe_zone = 100e3;
			}

			if (d > safe_zone)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[TacticalAI::SelectTarget] DROP abandoned ward Ship='%hs' Ward='%hs' Dist=%.2f Safe=%.2f"),
					ship ? ship->GetName() : "NULL",
					ward ? ward->GetName() : "NULL",
					d,
					safe_zone);

				ship_ai->DropTarget();
				return;
			}
		}
		else
		{
			if (d > safe_zone)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[TacticalAI::SelectTarget] HOLD near ward Ship='%hs' Ward='%hs' Dist=%.2f Safe=%.2f"),
					ship ? ship->GetName() : "NULL",
					ward ? ward->GetName() : "NULL",
					d,
					safe_zone);

				return;
			}
		}
	}

	//-------------------------------------------------------------
	// Existing target validation
	//-------------------------------------------------------------
	if (target)
	{
		if (target->GetLife())
		{
			CheckTarget();

			if (ship->GetClassification() != CLASSIFICATION::CORVETTE &&
				ship->GetClassification() != CLASSIFICATION::FRIGATE)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[TacticalAI::SelectTarget] KEEP existing target Ship='%hs' Target='%hs'"),
					ship ? ship->GetName() : "NULL",
					target ? target->GetName() : "NULL");

				return;
			}

			target = ship_ai->GetTarget();
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[TacticalAI::SelectTarget] DROP dead target Ship='%hs' Target='%hs'"),
				ship ? ship->GetName() : "NULL",
				target ? target->GetName() : "NULL");

			ship_ai->DropTarget();
			target = nullptr;
		}
	}

	//-------------------------------------------------------------
	// Target reacquisition delay
	//
	// TEMP UE MIGRATION:
	// Disable reacquisition blocking while
	// runtime tactical tracking stabilizes.
	//-------------------------------------------------------------
	if (ship_ai->DropTime() > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTarget] IGNORE DropTime Ship='%hs' DropTime=%.2f"),
			ship ? ship->GetName() : "NULL",
			ship_ai->DropTime());
	}

	//-------------------------------------------------------------
	// Directed ROE
	//-------------------------------------------------------------
	if (roe == DIRECTED)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTarget] DIRECTED Ship='%hs'"),
			ship ? ship->GetName() : "NULL");

		if (target &&
			target->GetType() == ESimObject::SHIP)
		{
			SelectTargetDirected((Ship*)target);
		}
		else
		{
			SelectTargetDirected();
		}
	}

	//-------------------------------------------------------------
	// Opportunity targeting
	//-------------------------------------------------------------
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTarget] OPPORTUNITY Ship='%hs' Contacts=%d"),
			ship ? ship->GetName() : "NULL",
			ship ? ship->GetContactList().size() : -1);

		SelectTargetOpportunity();

		if (ship->GetClassification() == CLASSIFICATION::CORVETTE ||
			ship->GetClassification() == CLASSIFICATION::FRIGATE)
		{
			SimObject* potential_target =
				ship_ai->GetTarget();

			if (target &&
				potential_target &&
				target != potential_target)
			{
				if (target->GetType() == ESimObject::SHIP &&
					potential_target->GetType() == ESimObject::SHIP)
				{
					ship_ai->SetTarget(target);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[TacticalAI::SelectTarget EXIT] Ship='%hs' FinalTarget='%hs'"),
		ship ? ship->GetName() : "NULL",
		ship_ai && ship_ai->GetTarget() ?
		ship_ai->GetTarget()->GetName() : "NULL");
}

// +--------------------------------------------------------------------+

void
TacticalAI::SelectTargetDirected(Ship* tgt)
{
	Ship* potential_target =
		tgt;

	//-------------------------------------------------------------
	// INSTRUCTION TARGET OVERRIDE
	//-------------------------------------------------------------
	if (!potential_target &&
		navpt &&
		navpt->GetTarget() &&
		navpt->GetTarget()->GetType() == ESimObject::SHIP)
	{
		potential_target =
			(Ship*)navpt->GetTarget();

		UE_LOG(LogTemp, Warning,
			TEXT("[TacticalAI::SelectTargetDirected] NAV TARGET Ship='%hs' Target='%hs'"),
			ship ? ship->GetName() : "NULL",
			potential_target ? potential_target->GetName() : "NULL");
	}

	//-------------------------------------------------------------
	// Element target objective fallback
	//-------------------------------------------------------------
	if (!potential_target)
	{
		SimElement* elem =
			ship ? ship->GetElement() : nullptr;

		if (elem)
		{
			Instruction* objective =
				elem->GetTargetObjective();

			if (objective)
			{
				SimObject* obj_sim_obj =
					objective->GetTarget();

				Ship* obj_tgt =
					nullptr;

				if (obj_sim_obj &&
					obj_sim_obj->GetType() == ESimObject::SHIP)
				{
					obj_tgt =
						(Ship*)obj_sim_obj;
				}

				//-------------------------------------------------
				// Direct validation
				//-------------------------------------------------
				if (obj_tgt &&
					obj_tgt != ship &&
					obj_tgt->GetIFF() != 0 &&
					obj_tgt->GetIFF() != ship->GetIFF() &&
					obj_tgt->GetLife() != 0 &&
					!obj_tgt->InTransition())
				{
					potential_target =
						obj_tgt;

					UE_LOG(LogTemp, Warning,
						TEXT("[TacticalAI::SelectTargetDirected] OBJECTIVE TARGET Ship='%hs' Target='%hs'"),
						ship ? ship->GetName() : "NULL",
						potential_target ?
						potential_target->GetName() : "NULL");
				}
			}
		}
	}

	//-------------------------------------------------------------
	// Migration-safe target validation
	//-------------------------------------------------------------
	if (potential_target &&
		potential_target != ship &&
		potential_target->GetLife() != 0 &&
		potential_target->GetIFF() != ship->GetIFF())
	{
		ship_ai->SetTarget(potential_target);
	}
	else
	{
		ship_ai->SetTarget(nullptr);
	}

	if (tgt && tgt == ship_ai->GetTarget())
	{
		directed_tgtid =
			tgt->GetIdentity();
	}
	else
	{
		directed_tgtid =
			0;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[TacticalAI::SelectTargetDirected EXIT] Ship='%hs' Input='%hs' Final='%hs' DirectedId=%d"),
		ship ? ship->GetName() : "NULL",
		tgt ? tgt->GetName() : "NULL",
		ship_ai && ship_ai->GetTarget() ?
		ship_ai->GetTarget()->GetName() : "NULL",
		directed_tgtid);
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
	if (!ship || !ship_ai)
	{
		return;
	}

	// NON-COMBATANTS do not pick targets of opportunity:
	if (ship->GetIFF() == 0)
	{
		return;
	}

	SimObject* potential_target = nullptr;

	ListIter<SimContact> contact =
		ship->GetContactList();

	while (++contact)
	{
		Ship* contact_ship =
			contact->GetShip();

		if (!contact_ship)
		{
			continue;
		}

		if (contact_ship == ship)
		{
			continue;
		}

		if (contact_ship->GetIFF() == ship->GetIFF())
		{
			continue;
		}

		if (contact_ship->GetIFF() == 0)
		{
			continue;
		}

		if (contact_ship->InTransition())
		{
			continue;
		}

		if (contact_ship->GetRegion() != ship->GetRegion())
		{
			continue;
		}

		if (contact->Threat(ship))
		{
			potential_target =
				contact_ship;

			break;
		}

		if (!potential_target)
		{
			potential_target =
				contact_ship;
		}
	}

	if (potential_target)
	{
		if (ship->GetClassification() != CLASSIFICATION::CARRIER &&
			ship->GetClassification() != CLASSIFICATION::SWACS)
		{
			ship_ai->SetTarget(
				potential_target);
		}
	}
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

	if (tgt->GetType() == ESimObject::SHIP) {
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
			if (directed_tgtid != target->GetIdentity()) {
				ship_ai->DropTarget();
			}

			return;
		}

		// can we catch the target?
		if (target->Design()->vlimit <= ship->Design()->vlimit ||
			ship->GetVelocity().Length() <= ship->Design()->vlimit)
			return;

		WeaponDesign* wep_dsn = ship->GetPrimaryDesign();
		if (!wep_dsn)
			return;

		double drop_range = 3 * wep_dsn->max_range;
		if (drop_range > 0.75 * ship->Design()->commit_range)
			drop_range = 0.75 * ship->Design()->commit_range;

		double range = (target->GetLocation() - ship->GetLocation()).Length();
		if (range < drop_range)
			return;

		FVector delta = (target->GetLocation() + target->GetVelocity()) -
			(ship->GetLocation() + ship->GetVelocity());

		if (delta.Length() < range)
			return;

		ship_ai->DropTarget();
	}
	else if (tgt->GetType() == ESimObject::DRONE) {
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

	ListIter<SimContact> iter = ship->GetContactList();

	while (++iter) {
		SimContact* contact = iter.value();

		if (contact->Threat(ship) &&
			(Game::GetGameTime() - contact->AcquisitionTime()) > THREAT_REACTION_TIME) {

			if (contact->GetShot()) {
				threat_missile = contact->GetShot();
				rumor = (Ship*)threat_missile->Owner();
			}
			else {
				double rng = contact->Range(ship);

				Ship* c_ship = contact->GetShip();
				if (c_ship && !c_ship->InTransition() &&
					c_ship->GetClassification() != CLASSIFICATION::FREIGHTER &&
					c_ship->GetClassification() != CLASSIFICATION::FARCASTER) {

					if (c_ship->GetTarget() == ship) {
						if (!threat || c_ship->GetClassification() > threat->GetClassification()) {
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

	ListIter<SimContact> contact = ship->GetContactList();

	while (++contact) {
		if (contact->GetShip() && contact->GetIFF(ship) == ship->GetIFF()) {
			Ship* c_ship = contact->GetShip();

			if (c_ship != ship && c_ship->GetClassification() >= ship->GetClassification() && !c_ship->InTransition()) {
				if (!support || c_ship->GetClassification() > support->GetClassification())
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

	ship_ai->SetFormationDelta(delta * ship->GetRadius() * 2);
}
