/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         FlightPlanner.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Flight Planning class for creating navpoint routes for fighter elements.
	Used both by the CarrierAI class and the Flight Dialog.
*/

#include "FlightPlanner.h"

#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
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

#include "Game.h"

#include "CoreMinimal.h" // UE_LOG, FMath
#include "Math/Vector.h" // FVector

// +----------------------------------------------------------------------+

FlightPlanner::FlightPlanner(Ship* s)
	: sim(nullptr), ship(s), patrol_range(0.0f)
{
	sim = Sim::GetSim();

	// Original: 250e3 + 10e3 * (int) Random(0, 8.9)
	// UE: inclusive ints, 0..8
	const int32 RandBucket = FMath::RandRange(0, 8);
	patrol_range = static_cast<float>(250e3 + 10e3 * RandBucket);
}

FlightPlanner::~FlightPlanner()
{
}

// +--------------------------------------------------------------------+

void
FlightPlanner::CreatePatrolRoute(SimElement* elem, int index)
{
	RLoc           rloc;
	FVector        dummy(0.0, 0.0, 0.0);
	FVector        loc = ship->Location();
	double         zone = ship->CompassHeading();
	Instruction* instr = nullptr;

	if (ship->IsAirborne())
		loc.Y += 8e3;
	else
		loc.Y += 1e3;

	if (index > 2)
		zone += 170 * DEGREES;
	else if (index > 1)
		zone += -90 * DEGREES;
	else if (index > 0)
		zone += 90 * DEGREES;

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(30e3);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(-10 * DEGREES + zone);
	rloc.SetAzimuthVar(0);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
	instr->SetSpeed(750);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	if (ship->IsAirborne())
		rloc.SetDistance(140e3);
	else
		rloc.SetDistance(220e3);
	rloc.SetDistanceVar(50e3);
	rloc.SetAzimuth(-20 * DEGREES + zone);
	rloc.SetAzimuthVar(15 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::PATROL);
	instr->SetSpeed(500);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	rloc.SetReferenceLoc(&instr->GetRLoc());
	rloc.SetDistance(120e3);
	rloc.SetDistanceVar(30e3);
	rloc.SetAzimuth(60 * DEGREES + zone);
	rloc.SetAzimuthVar(20 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::PATROL);
	instr->SetSpeed(350);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	rloc.SetReferenceLoc(&instr->GetRLoc());
	rloc.SetDistance(120e3);
	rloc.SetDistanceVar(30e3);
	rloc.SetAzimuth(120 * DEGREES + zone);
	rloc.SetAzimuthVar(20 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::PATROL);
	instr->SetSpeed(350);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(40e3);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(180 * DEGREES + ship->CompassHeading());
	rloc.SetAzimuthVar(0 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::RTB);
	instr->SetSpeed(500);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);
}

// +--------------------------------------------------------------------+

void
FlightPlanner::CreateStrikeRoute(SimElement* elem, SimElement* target)
{
	if (!elem)
		return;

	RLoc           rloc;
	FVector        dummy(0.0, 0.0, 0.0);
	FVector        loc = ship->Location();
	double         head = ship->CompassHeading() + 15 * DEGREES;
	double         dist = 30e3;
	Instruction* instr = nullptr;
	Ship* tgt_ship = nullptr;

	if (ship->IsAirborne())
		loc += ship->Cam().vup() * 8e3;
	else
		loc += ship->Cam().vup() * 1e3;;

	if (target)
		tgt_ship = target->GetShip(1);

	if (tgt_ship) {
		const double range = FVector(tgt_ship->Location() - ship->Location()).Size();

		if (range < 100e3)
			dist = 20e3;
	}

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(dist);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(head);
	rloc.SetAzimuthVar(2 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
	instr->SetSpeed(750);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	if (tgt_ship) {
		Ship* tgt_ship2 = target->GetShip(1);

		FVector tgt = tgt_ship2->Location() + tgt_ship2->Velocity() * 10;
		FVector mid = ship->Location() + (tgt - ship->Location()) * 0.5;
		double  beam = tgt_ship2->CompassHeading() + 90 * DEGREES;

		if (tgt_ship2->IsAirborne())
			tgt += tgt_ship2->Cam().vup() * 8e3;
		else
			tgt += tgt_ship2->Cam().vup() * 1e3;

		if (tgt_ship2 && tgt_ship2->IsStarship()) {
			rloc.SetReferenceLoc(nullptr);
			rloc.SetBaseLocation(tgt);
			rloc.SetDistance(60e3);
			rloc.SetDistanceVar(5e3);
			rloc.SetAzimuth(beam);
			rloc.SetAzimuthVar(5 * DEGREES);

			instr = new Instruction(tgt_ship2->GetRegion(), dummy, INSTRUCTION_ACTION::ASSAULT);
			instr->SetSpeed(750);
			instr->GetRLoc() = rloc;
			instr->SetTarget(FString(target->Name().data()));
			instr->SetFormation(INSTRUCTION_FORMATION::TRAIL);

			elem->AddNavPoint(instr);
		}

		if (tgt_ship2 && tgt_ship2->IsStatic()) {
			rloc.SetReferenceLoc(nullptr);
			rloc.SetBaseLocation(mid);
			rloc.SetDistance(60e3);
			rloc.SetDistanceVar(5e3);
			rloc.SetAzimuth(beam);
			rloc.SetAzimuthVar(15 * DEGREES);

			instr = new Instruction(tgt_ship2->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
			instr->SetSpeed(750);
			instr->GetRLoc() = rloc;

			elem->AddNavPoint(instr);

			rloc.SetReferenceLoc(nullptr);
			rloc.SetBaseLocation(tgt);
			rloc.SetDistance(40e3);
			rloc.SetDistanceVar(5e3);
			rloc.SetAzimuth(beam);
			rloc.SetAzimuthVar(5 * DEGREES);

			INSTRUCTION_ACTION action = INSTRUCTION_ACTION::ASSAULT;

			if (tgt_ship2->IsGroundUnit())
				action = INSTRUCTION_ACTION::STRIKE;

			instr = new Instruction(tgt_ship2->GetRegion(), dummy, action);
			instr->SetSpeed(750);
			instr->GetRLoc() = rloc;
			instr->SetTarget(FString(target->Name().data()));
			instr->SetFormation(INSTRUCTION_FORMATION::TRAIL);

			elem->AddNavPoint(instr);
		}

		else if (tgt_ship2 && tgt_ship2->IsDropship()) {
			rloc.SetReferenceLoc(nullptr);
			rloc.SetBaseLocation(tgt);
			rloc.SetDistance(60e3);
			rloc.SetDistanceVar(5e3);
			rloc.SetAzimuth(tgt_ship2->CompassHeading());
			rloc.SetAzimuthVar(20 * DEGREES);

			instr = new Instruction(tgt_ship2->GetRegion(), dummy, INSTRUCTION_ACTION::INTERCEPT);
			instr->SetSpeed(750);
			instr->GetRLoc() = rloc;
			instr->SetTarget(FString(target->Name().data()));
			instr->SetFormation(INSTRUCTION_FORMATION::SPREAD);

			elem->AddNavPoint(instr);
		}
	}

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(40e3);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(180 * DEGREES + ship->CompassHeading());
	rloc.SetAzimuthVar(0 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::RTB);
	instr->SetSpeed(500);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);
}

// +--------------------------------------------------------------------+

void
FlightPlanner::CreateEscortRoute(SimElement* elem, SimElement* ward)
{
	if (!elem)
		return;

	RLoc           rloc;
	FVector        dummy(0.0, 0.0, 0.0);
	FVector        loc = ship->Location();
	double         head = ship->CompassHeading();
	Instruction* instr = nullptr;

	if (ship->IsAirborne())
		loc += ship->Cam().vup() * 8e3;
	else
		loc += ship->Cam().vup() * 1e3;
	
	//loc = loc.OtherHand();

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(30e3);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(head);
	rloc.SetAzimuthVar(0);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::VECTOR);
	instr->SetSpeed(750);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);

	if (ward && ward->GetShip(1)) {
		// follow ward's flight plan:
		if (ward->GetFlightPlan().size()) {
			ListIter<Instruction> iter = ward->GetFlightPlan();

			while (++iter) {
				Instruction* ward_instr = iter.value();

				if (ward_instr->GetAction() != INSTRUCTION_ACTION::RTB) {
					rloc.SetReferenceLoc(&ward_instr->GetRLoc());
					rloc.SetDistance(25e3);
					rloc.SetDistanceVar(5e3);
					rloc.SetAzimuth(0);
					rloc.SetAzimuthVar(90 * DEGREES);

					instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::ESCORT);
					instr->SetSpeed(350);
					instr->GetRLoc() = rloc;
					instr->SetTarget(FString(ward->Name().data()));
					elem->AddNavPoint(instr);
				}
			}
		}

		// if ward has no flight plan, just go to a point nearby:
		else {
			rloc.SetReferenceLoc(nullptr);
			rloc.SetBaseLocation(ward->GetShip(1)->Location());
			rloc.SetDistance(25e3);
			rloc.SetDistanceVar(5e3);
			rloc.SetAzimuth(0);
			rloc.SetAzimuthVar(90 * DEGREES);

			instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::DEFEND);
			instr->SetSpeed(500);
			instr->GetRLoc() = rloc;
			instr->SetTarget(FString(ward->Name().data()));
			instr->SetHoldTime(15 * 60); // fifteen minutes

			elem->AddNavPoint(instr);
		}
	}

	rloc.SetReferenceLoc(nullptr);
	rloc.SetBaseLocation(loc);
	rloc.SetDistance(40e3);
	rloc.SetDistanceVar(0);
	rloc.SetAzimuth(180 * DEGREES + ship->CompassHeading());
	rloc.SetAzimuthVar(0 * DEGREES);

	instr = new Instruction(ship->GetRegion(), dummy, INSTRUCTION_ACTION::RTB);
	instr->SetSpeed(500);
	instr->GetRLoc() = rloc;

	elem->AddNavPoint(instr);
}

