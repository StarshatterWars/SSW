/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Farcaster.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
*/

#include "Farcaster.h"

#include "QuantumDrive.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Explosion.h"
#include "Sim.h"
#include "SimElement.h"
#include "Instruction.h"

#include "Game.h"
#include "Solid.h"
#include "SimLight.h"
#include "Sound.h"
#include "DataLoader.h"

// Unreal minimal includes for FVector + random:
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Matrix.h"

// +======================================================================+

Farcaster::Farcaster(double cap, double rate)
	: SimSystem(SYSTEM_CATEGORY::FARCASTER, 0, "Farcaster", 1, (float)cap, (float)cap, (float)rate),
	ship(0),
	dest(0),
	jumpship(0),
	cycle_time(10),
	active_state(QuantumDrive::ACTIVE_READY),
	warp_fov(1),
	no_dest(false)
{
	name = "Farcaster";
	abrv = "Farcaster";
}

// +----------------------------------------------------------------------+

Farcaster::Farcaster(const Farcaster& s)
	: SimSystem(s),
	ship(0),
	dest(0),
	start_rel(s.start_rel),
	end_rel(s.end_rel),
	jumpship(0),
	cycle_time(s.cycle_time),
	active_state(QuantumDrive::ACTIVE_READY),
	warp_fov(1),
	no_dest(false)
{
	Mount(s);
	SetAbbreviation(s.Abbreviation());

	for (int i = 0; i < NUM_APPROACH_PTS; i++)
		approach_rel[i] = s.approach_rel[i];
}

// +--------------------------------------------------------------------+

Farcaster::~Farcaster()
{
}

// +--------------------------------------------------------------------+

void
Farcaster::ExecFrame(double seconds)
{
	SimSystem::ExecFrame(seconds);

	if (ship && !no_dest) {
		if (!dest) {
			SimElement* elem = ship->GetElement();

			if (elem && elem->NumObjectives()) {
				Sim* sim = Sim::GetSim();
				Instruction* obj = elem->GetObjective(0);

				if (sim && obj)
					dest = sim->FindShip(obj->TargetName());
			}

			if (!dest)
				no_dest = true;
		}
		else {
			if (dest->IsDying() || dest->IsDead()) {
				dest = 0;
				no_dest = true;
			}
		}
	}

	// if no destination, show red nav lights:
	if (no_dest)
		energy = 0.0f;

	if (active_state == QuantumDrive::ACTIVE_READY && energy >= capacity &&
		ship && ship->GetRegion() && dest && dest->GetRegion()) {

		SimRegion* rgn = ship->GetRegion();
		ListIter<Ship> s_iter = rgn->GetShips();

		jumpship = 0;

		while (++s_iter) {
			Ship* s = s_iter.value();

			if (s == ship || s->IsStatic() || s->WarpFactor() > 1)
				continue;

			FVector delta = s->Location() - ship->Location();

			// activate:
			if (delta.Length() < 1000.0f) {
				active_state = QuantumDrive::ACTIVE_PREWARP;
				jumpship = s;
				Observe(jumpship);
				break;
			}
		}
	}

	if (active_state == QuantumDrive::ACTIVE_READY)
		return;

	if (ship) {
		bool warping = false;

		if (active_state == QuantumDrive::ACTIVE_PREWARP) {
			if (warp_fov < 5000) {
				warp_fov *= 1.5;
			}
			else {
				Jump();
			}

			warping = true;
		}

		else if (active_state == QuantumDrive::ACTIVE_POSTWARP) {
			if (warp_fov > 1) {
				warp_fov *= 0.75;
			}
			else {
				warp_fov = 1;
				active_state = QuantumDrive::ACTIVE_READY;
			}

			warping = true;
		}

		if (jumpship) {
			if (warping) {
				jumpship->SetWarp(warp_fov);

				SimRegion* r = ship->GetRegion();
				ListIter<Ship> neighbor = r->GetShips();

				while (++neighbor) {
					if (neighbor->IsDropship()) {
						Ship* s = neighbor.value();
						FVector d = s->Location() - ship->Location();

						if (d.Length() < 5e3)
							s->SetWarp(warp_fov);
					}
				}
			}
			else {
				warp_fov = 1;
				jumpship->SetWarp(warp_fov);
			}
		}
	}
}

void
Farcaster::Jump()
{
	Sim* sim = Sim::GetSim();
	if (!sim || !ship || !dest || !jumpship)
		return;

	SimRegion* rgn = ship->GetRegion();
	SimRegion* dst = dest->GetRegion();

	sim->CreateExplosion(jumpship->Location(), FVector::ZeroVector,
		Explosion::QUANTUM_FLASH, 1.0f, 0, rgn);

	// NOTE:
	// Original code used: dest->Location().OtherHand()
	// That helper is not available once Point is converted to FVector.
	// Using dest->Location() as the best safe equivalent here (destination position in the dst region).
	sim->RequestHyperJump(jumpship, dst, dest->Location(), 0, ship, dest);

	energy = 0.0f;

	Farcaster* f = dest->GetFarcaster();
	if (f)
		f->Arrive(jumpship);

	active_state = QuantumDrive::ACTIVE_READY;
	warp_fov = 1;
	jumpship = 0;
}

void
Farcaster::Arrive(Ship* s)
{
	energy = 0.0f;

	active_state = QuantumDrive::ACTIVE_POSTWARP;
	warp_fov = 5000;
	jumpship = s;

	if (jumpship && jumpship->Velocity().Length() < 500.0f) {
		jumpship->SetVelocity(jumpship->Heading() * 500.0f);
	}
}

// +----------------------------------------------------------------------+

void
Farcaster::SetApproachPoint(int i, FVector loc)
{
	if (i >= 0 && i < NUM_APPROACH_PTS)
		approach_rel[i] = loc;
}

void
Farcaster::SetStartPoint(FVector loc)
{
	start_rel = loc;
}

void
Farcaster::SetEndPoint(FVector loc)
{
	end_rel = loc;
}

// +----------------------------------------------------------------------+

void
Farcaster::SetCycleTime(double t)
{
	cycle_time = t;
}

// +----------------------------------------------------------------------+

void
Farcaster::Orient(const Physical* rep)
{
	SimSystem::Orient(rep);

	const Matrix& legacyOrientation = rep->Cam().Orientation();
	const FVector  loc = rep->Location();

	const FMatrix Orientation(
		FPlane(legacyOrientation(0, 0), legacyOrientation(0, 1), legacyOrientation(0, 2), 0.0f),
		FPlane(legacyOrientation(1, 0), legacyOrientation(1, 1), legacyOrientation(1, 2), 0.0f),
		FPlane(legacyOrientation(2, 0), legacyOrientation(2, 1), legacyOrientation(2, 2), 0.0f),
		FPlane(0, 0, 0, 1)
	);

	start_point = Orientation.TransformPosition(start_rel) + loc;
	end_point = Orientation.TransformPosition(end_rel) + loc;

	for (int32 i = 0; i < NUM_APPROACH_PTS; ++i)
	{
		approach_point[i] =
			Orientation.TransformPosition(approach_rel[i]) + loc;
	}
}

// +----------------------------------------------------------------------+

bool
Farcaster::Update(SimObject* obj)
{
	if (obj == jumpship) {
		jumpship->SetWarp(1);

		SimRegion* r = ship->GetRegion();
		ListIter<Ship> neighbor = r->GetShips();

		while (++neighbor) {
			if (neighbor->IsDropship()) {
				Ship* s = neighbor.value();
				FVector d = s->Location() - ship->Location();

				if (d.Length() < 5e3)
					s->SetWarp(1);
			}
		}

		jumpship = 0;
	}

	return SimObserver::Update(obj);
}

const char*
Farcaster::GetObserverName() const
{
	return Name();
}
