/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         CarrierAI.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	"Air Boss" AI class for managing carrier fighter squadrons
*/

#include "CarrierAI.h"

#include "ShipAI.h"
#include "Ship.h"
#include "ShipLoad.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "FlightPlanner.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "Mission.h"
#include "SimContact.h"
#include "Sim.h"
#include "StarSystem.h"
#include "Callsign.h"
#include "GameStructs.h"
#include "Game.h"
#include "Random.h"

// Unreal (for UE_LOG)
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogTempCarrierAI, Log, All);

// +----------------------------------------------------------------------+

CarrierAI::CarrierAI(Ship* s, int level)
	: sim(0), ship(s), hangar(0), exec_time(0), flight_planner(0),
	hold_time(0), ai_level(level)
{
	if (ship) {
		sim = Sim::GetSim();
		hangar = ship->GetHangar();

		for (int i = 0; i < 4; i++)
			patrol_elem[i] = 0;

		if (ship)
			flight_planner = new FlightPlanner(ship);

		hold_time = (int)Game::GameTime();
	}
}

CarrierAI::~CarrierAI()
{
	delete flight_planner;
}

// +--------------------------------------------------------------------+

void
CarrierAI::ExecFrame(double secs)
{
	const int INIT_HOLD = 15000;
	const int EXEC_PERIOD = 3000;

	if (!sim || !ship || !hangar)
		return;

	if (((int)Game::GameTime() - hold_time >= INIT_HOLD) &&
		((int)Game::GameTime() - exec_time > EXEC_PERIOD)) {

		CheckHostileElements();
		CheckPatrolCoverage();

		exec_time = (int)Game::GameTime();
	}
}

// +--------------------------------------------------------------------+

bool
CarrierAI::CheckPatrolCoverage()
{
	const DWORD PATROL_PERIOD = 900 * 1000;

	// pick up existing patrol elements:

	ListIter<SimElement> iter = sim->GetElements();
	while (++iter) {
		SimElement* elem = iter.value();

		if (elem->GetCarrier() == ship &&
			(elem->Type() == (int)EMISSIONTYPE::PATROL ||
				elem->Type() == (int)EMISSIONTYPE::SWEEP ||
				elem->Type() == (int)EMISSIONTYPE::AIR_PATROL ||
				elem->Type() == (int)EMISSIONTYPE::AIR_SWEEP) &&
			!elem->IsSquadron() &&
			!elem->IsFinished()) {

			bool found = false;
			int  open = -1;

			for (int i = 0; i < 4; i++) {
				if (patrol_elem[i] == elem)
					found = true;

				else if (patrol_elem[i] == 0 && open < 0)
					open = i;
			}

			if (!found && open >= 0) {
				patrol_elem[open] = elem;
			}
		}
	}

	// manage the four screening patrols:

	for (int i = 0; i < 4; i++) {
		SimElement* elem = patrol_elem[i];

		if (elem) {
			if (elem->IsFinished()) {
				patrol_elem[i] = 0;
			}
			else {
				LaunchElement(elem);
			}
		}

		else if (Game::GameTime() - hangar->GetLastPatrolLaunch() > PATROL_PERIOD ||
			hangar->GetLastPatrolLaunch() == 0) {
			SimElement* patrol = CreatePackage(0, 2, (int)EMISSIONTYPE::PATROL, 0, "ACM Medium Range");
			if (patrol) {
				patrol_elem[i] = patrol;

				if (flight_planner)
					flight_planner->CreatePatrolRoute(patrol, i);

				hangar->SetLastPatrolLaunch(Game::GameTime());
				return true;
			}
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
CarrierAI::CheckHostileElements()
{
	List<SimElement>     assigned;
	ListIter<SimElement> iter = sim->GetElements();
	while (++iter) {
		SimElement* elem = iter.value();

		// if this element is hostile to us
		// or if the element is a target objective
		// of the carrier, or is hostile to any
		// of our squadrons...

		bool hostile = false;

		if (elem->IsHostileTo(ship) || elem->IsObjectiveTargetOf(ship)) {
			hostile = true;
		}
		else {
			for (int i = 0; i < hangar->NumSquadrons() && !hostile; i++) {
				int squadron_iff = hangar->SquadronIFF(i);

				if (elem->IsHostileTo(squadron_iff))
					hostile = true;
			}
		}

		if (hostile) {
			sim->GetAssignedElements(elem, assigned);

			// is one of our fighter elements already assigned to this target?
			bool found = false;
			ListIter<SimElement> a_iter = assigned;
			while (++a_iter && !found) {
				SimElement* a = a_iter.value();

				if (a->GetCarrier() == ship)
					found = true;
			}

			// nobody is assigned yet, create an attack package
			if (!found && CreateStrike(elem)) {
				hold_time = (int)Game::GameTime() + 30000;
				return true;
			}
		}
	}

	return false;
}

bool
CarrierAI::CreateStrike(SimElement* elem)
{
	SimElement* strike = 0;
	Ship* target = elem->GetShip(1);

	if (target && !target->IsGroundUnit()) {
		SimContact* contact = ship->FindContact(target);
		if (contact && contact->GetIFF(ship) > 0) {

			// fighter intercept
			if (target->IsDropship()) {
				int squadron = 0;
				if (hangar->NumShipsReady(1) >= hangar->NumShipsReady(0))
					squadron = 1;

				int count = 2;

				if (count < elem->NumShips())
					count = elem->NumShips();

				strike = CreatePackage(squadron, count, (int)EMISSIONTYPE::INTERCEPT, elem->Name(), "ACM Medium Range");

				if (strike) {
					strike->SetAssignment(elem);

					if (flight_planner)
						flight_planner->CreateStrikeRoute(strike, elem);
				}
			}

			// starship or station assault
			else {
				int squadron = 0;
				if (hangar->NumSquadrons() > 1)
					squadron = 1;
				if (hangar->NumSquadrons() > 2)
					squadron = 2;

				int count = 2;

				if (target->Class() > CLASSIFICATION::FRIGATE) {
					count = 4;
					strike = CreatePackage(squadron, count, (int)EMISSIONTYPE::ASSAULT, elem->Name(), "Hvy Ship Strike");
				}
				else {
					count = 2;
					strike = CreatePackage(squadron, count, (int)EMISSIONTYPE::ASSAULT, elem->Name(), "Ship Strike");
				}

				if (strike) {
					strike->SetAssignment(elem);

					if (flight_planner)
						flight_planner->CreateStrikeRoute(strike, elem);

					// strike escort if target has fighter protection:
					if (target->GetHangar()) {
						if (squadron > 1) squadron--;
						SimElement* escort = CreatePackage(squadron, 2, (int)EMISSIONTYPE::ESCORT_STRIKE, strike->Name(), "ACM Short Range");

						if (escort && flight_planner)
							flight_planner->CreateEscortRoute(escort, strike);
					}
				}
			}
		}
	}

	return strike != 0;
}

// +--------------------------------------------------------------------+

SimElement*
CarrierAI::CreatePackage(int SquadronIndex, int PackageSize, int MissionCode, const char* Target, const char* LoadoutName)
{
	if (SquadronIndex < 0 ||
		PackageSize < 1 ||
		MissionCode < (int)EMISSIONTYPE::PATROL ||
		hangar->NumShipsReady(SquadronIndex) < PackageSize)
	{
		return nullptr;
	}

	// Fix C4458: avoid shadowing CarrierAI::sim (class member) by using a different local name.
	Sim* SimContext = Sim::GetSim();

	const char* Callsign = SimContext->FindAvailCallsign(ship->GetIFF());
	SimElement* Element = SimContext->CreateElement(Callsign, ship->GetIFF(), MissionCode);

	FlightDeck* SelectedDeck = nullptr;
	int QueueScore = 1000;

	int* Loadout = nullptr;
	const ShipDesign* SquadronDesign = hangar->SquadronDesign(SquadronIndex);

	Element->SetSquadron(hangar->SquadronName(SquadronIndex));
	Element->SetCarrier(ship);

	if (Target) {
		INSTRUCTION_ACTION InstructionCode = INSTRUCTION_ACTION::VECTOR;

		switch (MissionCode) {
		case (int)EMISSIONTYPE::ASSAULT:
			InstructionCode = INSTRUCTION_ACTION::ASSAULT;  
			break;

		case (int)EMISSIONTYPE::STRIKE:
			InstructionCode = INSTRUCTION_ACTION::STRIKE;   
			break;

		case (int)EMISSIONTYPE::AIR_INTERCEPT:
		case (int)EMISSIONTYPE::INTERCEPT:
			InstructionCode = INSTRUCTION_ACTION::INTERCEPT; 
			break;

		case (int)EMISSIONTYPE::ESCORT:
		case (int)EMISSIONTYPE::ESCORT_STRIKE:
		case (int)EMISSIONTYPE::ESCORT_FREIGHT:
			InstructionCode = INSTRUCTION_ACTION::ESCORT; 
			break;

		case (int)EMISSIONTYPE::DEFEND:
			InstructionCode = INSTRUCTION_ACTION::DEFEND;
			break;

		default:
			break;
		}

		Instruction* Objective = new Instruction(InstructionCode, Target);
		if (Objective) {
			Element->AddObjective(Objective);
		}
	}

	if (SquadronDesign && LoadoutName) {
		Text Name = LoadoutName;
		Name.setSensitive(false);

		ListIter<ShipLoad> ShipLoads = (List<ShipLoad>&)SquadronDesign->loadouts;

		while (++ShipLoads)
		{
			if (Name == ShipLoads->GetName())
			{
				for (int i = 0; i < 16; i++)
				{
					Loadout[i] = ShipLoads->GetStation(i);
				}

				Element->SetLoadout(Loadout);
				break;
			}
		}
	}

	for (int DeckIndex = 0; DeckIndex < ship->NumFlightDecks(); DeckIndex++) {
		FlightDeck* Deck = ship->GetFlightDeck(DeckIndex);

		if (Deck && Deck->IsLaunchDeck()) {
			const int DeckQueue = hangar->PreflightQueue(Deck);

			if (DeckQueue < QueueScore) {
				QueueScore = DeckQueue;
				SelectedDeck = Deck;
			}
		}
	}

	int NumInPackage = 0;
	int Slots[4];

	for (int i = 0; i < 4; i++) {
		Slots[i] = -1;
	}

	for (int SlotIndex = 0; SlotIndex < hangar->SquadronSize(SquadronIndex); SlotIndex++) {
		const HangarSlot* Slot = hangar->GetSlot(SquadronIndex, SlotIndex);

		if (hangar->GetState(Slot) == Hangar::STORAGE) {
			if (NumInPackage < 4) {
				Slots[NumInPackage] = SlotIndex;
			}

			hangar->GotoAlert(SquadronIndex, SlotIndex, SelectedDeck, Element, Loadout, MissionCode > (int)EMISSIONTYPE::SWEEP);
			NumInPackage++;

			if (NumInPackage >= PackageSize) {
				break;
			}
		}
	}

	return Element;
}

// +--------------------------------------------------------------------+

bool
CarrierAI::LaunchElement(SimElement* elem)
{
	bool result = false;

	if (!elem)
		return result;

	for (int squadron = 0; squadron < hangar->NumSquadrons(); squadron++) {
		for (int slot = 0; slot < hangar->SquadronSize(squadron); slot++) {
			const HangarSlot* s = hangar->GetSlot(squadron, slot);

			if (hangar->GetState(s) == Hangar::ALERT &&
				hangar->GetPackageElement(s) == elem) {

				hangar->Launch(squadron, slot);

				result = true;
			}
		}
	}

	return result;
}

