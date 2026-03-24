/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         DropShipAI.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Drop Ship (orbit/surface and surface/orbit) AI class
*/

#include "DropShipAI.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "TacticalAI.h"
#include "Ship.h"
#include "ShipManager.h"
#include "Drive.h"
#include "Sim.h"
#include "StarSystem.h"
#include "KeyMap.h"

#include "Game.h"


// +----------------------------------------------------------------------+

DropShipAI::DropShipAI(Ship* s)
	: ShipAI(s)
{
	seek_gain = 20;
	seek_damp = 0.5;

	delete tactical;
	tactical = 0;
}

DropShipAI::~DropShipAI()
{
}

// +--------------------------------------------------------------------+

void
DropShipAI::FindObjective()
{
	distance = 0;

	if (!ship || !self)
		return;

	Sim* sim = Sim::GetSim();
	if (!sim)
		return;

	SimRegion* self_rgn = ship->GetRegion();
	if (!self_rgn)
		return;

	// If making orbit, go up:
	if (self_rgn->GetType() == Sim::AIR_SPACE) {
		obj_w = self->Location() + FVector(0.0f, 1.0e3f, 0.0f);
	}

	// If breaking orbit, head for terrain region:
	else {
		SimRegion* dst_rgn = sim->FindNearestTerrainRegion(ship);
		if (!dst_rgn)
			return;

		OrbitalRegion* dst_orb = dst_rgn->GetOrbitalRegion();
		OrbitalRegion* self_orb = self_rgn->GetOrbitalRegion();

		SimRegion* dst_orb_rgn = sim->FindRegion(dst_orb);
		SimRegion* self_orb_rgn = sim->FindRegion(self_orb);

		// Guard against null orbital regions:
		const FVector DstOrbLoc = dst_orb ? dst_orb->Location() : FVector::ZeroVector;
		const FVector SelfOrbLoc = self_orb ? self_orb->Location() : FVector::ZeroVector;

		const FVector dst =
			(DstOrbLoc - SelfOrbLoc) +
			FVector(0.0f, 0.0f, -1.0e6f);

		// Starshatter "OtherHand()" likely flips handedness; replace with your project helper.
		// If you don't have one yet, keep destination as-is and log once for verification.
		// obj_w = dst.OtherHand();
		obj_w = dst;
	}

	// Distance from self to navpt:
	distance = (obj_w - self->Location()).Size();

	// Transform into local/camera coords:
	objective = Transform(obj_w);

	// UE-safe normalize:
	objective = objective.GetSafeNormal();
}

// +--------------------------------------------------------------------+

void
DropShipAI::Navigator()
{
	accumulator.Clear();
	magnitude = 0;

	if (!ship || !self)
		return;

	if (other)
		ship->SetFLCSMode(Ship::FLCS_AUTO);
	else
		ship->SetFLCSMode(Ship::FLCS_MANUAL);

	Accumulate(AvoidCollision());
	Accumulate(Seek(objective));

	// Are we being asked to flee?
	// NOTE: Equality checks on floats can be brittle; preserve legacy intent but prefer tolerance if desired.
	if (FMath::Abs(accumulator.yaw) == 1.0f && accumulator.pitch == 0.0f) {
		accumulator.pitch = -0.7f;
		accumulator.yaw *= 0.25f;
	}

	self->ApplyRoll((float)(accumulator.yaw * -0.4f));
	self->ApplyYaw((float)(accumulator.yaw * 0.2f));

	if (FMath::Abs(accumulator.yaw) > 0.5f && FMath::Abs(accumulator.pitch) < 0.1f)
		accumulator.pitch -= 0.1f;

	if (accumulator.pitch != 0.0f)
		self->ApplyPitch((float)accumulator.pitch);

	// If not turning, roll to orient with world coords:
	if (FMath::Abs(accumulator.yaw) < 0.1f) {

		// Legacy code assumed Camera::vrt() with .y and .length()
		// UE conversion: use FVector::Size() and explicit component access.
		// Keep the original call pattern, but make it safe.
		Camera* Cam = (Camera*)&(self->Cam());
		if (Cam) {
			const FVector Vrt = Cam->vrt();
			const double  Deflection = (double)Vrt.Y;

			const double Len = (double)Vrt.Size();
			if (Len > KINDA_SMALL_NUMBER && Deflection != 0.0) {
				const double Ratio = FMath::Clamp(Deflection / Len, -1.0, 1.0);
				const double Theta = FMath::Asin(Ratio);
				self->ApplyRoll(-Theta);
			}
		}
	}

	ship->SetThrottle(100);
	ship->ExecFLCSFrame();
}