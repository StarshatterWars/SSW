/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Drone.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Decoy / Weapons Drone class implementation
*/

#include "Drone.h"

#include "Weapon.h"
#include "Ship.h"
#include "Sim.h"
#include "Explosion.h"

#include "Game.h"
#include "Bolt.h"
#include "Sprite.h"
#include "Solid.h"
#include "SimLight.h"

// Bitmap replaced with UE assets in the renderer layer (no include here)
#include "DataLoader.h"
#include "Sound.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

Drone::Drone(const FVector& pos, const Camera& shot_cam, WeaponDesign* dsn, const Ship* ship)
	: SimShot(pos, shot_cam, dsn, ship),
	iff_code(0),
	decoy_type(0),
	probe(0)
{
	obj_type = SimObject::SIM_DRONE;

	if (dsn) {
		decoy_type = dsn->decoy_type;
		probe = dsn->probe;
		integrity = dsn->integrity;

		// name is a legacy char buffer on the SimObject side:
		sprintf_s(name, "Drone %04d", Identity());
	}
}

// +--------------------------------------------------------------------+

Drone::~Drone()
{
}

// +--------------------------------------------------------------------+

void
Drone::SeekTarget(SimObject* target, SimSystem* sub)
{
	if (!probe) {
		SimShot::SeekTarget(target, sub);
	}
}

// +--------------------------------------------------------------------+

void
Drone::ExecFrame(double seconds)
{
	SimShot::ExecFrame(seconds);
}

// +--------------------------------------------------------------------+

void
Drone::Disarm()
{
	SimShot::Disarm();
}

// +--------------------------------------------------------------------+

void
Drone::Destroy()
{
	SimShot::Destroy();
}

// +--------------------------------------------------------------------+

double
Drone::PCS() const
{
	if (decoy_type == 0 && !probe)
		return 10e3;

	return 0;
}

double
Drone::ACS() const
{
	if (decoy_type == 0 && !probe)
		return 1e3;

	return 0;
}

// +--------------------------------------------------------------------+

const char*
Drone::ClassName() const
{
	return Ship::GetShipClassName(decoy_type);
}

int
Drone::Class() const
{
	return decoy_type;
}

// +--------------------------------------------------------------------+

int
Drone::HitBy(SimShot* shot, FVector& impact)
{
	if (life == 0 || !shot || !shot->IsArmed())
		return 0;

	const int HIT_NOTHING = 0;
	const int HIT_HULL = 1;

	FVector  hull_impact(0.0f, 0.0f, 0.0f);
	int      hit_type = HIT_NOTHING;

	const FVector shot_loc = shot->Location();
	const FVector shot_org = shot->Origin();
	const FVector delta = shot_loc - Location();
	const double  dlen = (double)delta.Length();

	double dscale = 1.0;

	float scale = design ? design->explosion_scale : 0.0f;
	Sim* sim = Sim::GetSim();

	if (!sim || !design)
		return HIT_NOTHING;

	if (scale <= 0)
		scale = design->scale;

	// MISSILE PROCESSING ------------------------------------------------

	if (shot->IsMissile()) {
		if (dlen < 10.0 * Radius()) {
			hull_impact = impact = shot_loc;
			sim->CreateExplosion(impact, Velocity(), Explosion::HULL_FLASH, 0.3f * scale, scale, region);
			sim->CreateExplosion(impact, FVector(0.0f, 0.0f, 0.0f), Explosion::SHOT_BLAST, 2.0f, scale, region);
			hit_type = HIT_HULL;
		}
	}

	// ENERGY WEP PROCESSING ---------------------------------------------

	else {
		if (shot->IsBeam()) {
			// check right-angle distance from beam line:
			const FVector d0 = Location() - shot_org;
			FVector       w = shot_loc - shot_org;
			w.Normalize();

			const FVector test = shot_org + w * FVector::DotProduct(d0, w);
			const FVector d1 = test - Location();
			const double  dist_from_line = (double)d1.Length();

			if (dist_from_line < 2.0 * Radius()) {
				hull_impact = impact = test;

				shot->SetBeamPoints(shot_org, impact);
				sim->CreateExplosion(impact, Velocity(), Explosion::BEAM_FLASH, 0.30f * scale, scale, region);
				hit_type = HIT_HULL;
			}
		}
		else if (dlen < 2.0 * Radius()) {
			hull_impact = impact = shot_loc;
			sim->CreateExplosion(impact, Velocity(), Explosion::HULL_FLASH, 0.30f * scale, scale, region);
			hit_type = HIT_HULL;
		}
	}

	// DAMAGE RESOLUTION -------------------------------------------------

	if (hit_type != HIT_NOTHING) {
		double effective_damage = shot->Damage() * dscale;

		if (shot->IsBeam()) {
			effective_damage *= Game::FrameTime();
		}
		else {
			ApplyTorque(shot->Velocity() * (float)effective_damage * 1e-6f);
		}

		if (effective_damage > 0) {
			Physical::InflictDamage(effective_damage);
		}
	}

	return hit_type;
}
