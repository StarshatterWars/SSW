/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Debris.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Debris Sprite animation class
*/

#include "Debris.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "SimShot.h"
#include "Explosion.h"
#include "Sim.h"
#include "StarSystem.h"
#include "Terrain.h"

#include "Solid.h"
#include "DataLoader.h"
#include "Game.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

Debris::Debris(SimModel* model, const FVector& pos, const FVector& vel, double m)
	: SimObject("Debris", ESimObject::DEBRIS)
{
	MoveTo(pos);

	velocity = vel;
	mass = (float)m;
	integrity = mass * 10.0f;
	life = 300;

	Solid* solid = new Solid;

	if (solid) {
		solid->UseModel(model);
		solid->MoveTo(pos);

		rep = solid;

		radius = solid->Radius();
	}

	FVector torque = FVector(
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f),
		FMath::FRandRange(-1.0f, 1.0f)
	);

	if (!torque.IsNearlyZero())
		torque.Normalize();

	torque *= (float)(GetMass() / 2.0);

	if (GetMass() < 10.0) {
		torque *= (FMath::FRand() / 3.2f);
	}
	else if (GetMass() > 10e6) {
		torque *= 0.005f;
	}
	else if (GetMass() > 10e3) {
		torque *= 0.25f;
	}

	ApplyTorque(torque);
}

// +--------------------------------------------------------------------+

int
Debris::HitBy(SimShot* shot, FVector& impact)
{
	if (!shot || !shot->IsArmed())
		return 0;

	const int HIT_NOTHING = 0;
	const int HIT_HULL = 1;

	FVector hull_impact = FVector::ZeroVector;
	int     hit_type = HIT_NOTHING;
	bool    hit_hull = true;

	const FVector shot_loc = shot->GetLocation();
	const FVector delta = shot_loc - GetLocation();
	const double  dlen = delta.Length();
	const double  dscale = 1.0;
	const float   scale = 1.0f;

	Sim* sim = Sim::GetSim();

	// MISSILE PROCESSING ------------------------------------------------

	if (shot->IsMissile()) {
		if (dlen < GetRadius()) {
			hull_impact = impact = shot_loc;

			if (sim) {
				sim->CreateExplosion(impact, GetVelocity(), EExplosionType::HULL_FLASH, 0.3f * scale, scale, region, this);
				sim->CreateExplosion(impact, FVector::ZeroVector, EExplosionType::SHOT_BLAST, 2.0f, scale, region);
			}

			hit_type = HIT_HULL;
		}
	}

	// ENERGY WEP PROCESSING ---------------------------------------------

	else {
		Solid* solid = (Solid*)rep;

		const FVector shot_vpn_raw = shot_loc - shot->GetOrigin();
		FVector       shot_vpn = shot_vpn_raw;
		double        shot_len = shot_vpn.Normalize();

		if (shot_len == 0.0)
			shot_len = 1000.0;

		// impact:
		if (solid) {
			if (solid->CheckRayIntersection(shot->GetOrigin(), shot_vpn, shot_len, impact)) {
				// trim beam shots to impact point:
				if (shot->IsBeam())
					shot->SetBeamPoints(shot->GetOrigin(), impact);

				hull_impact = impact;

				if (sim) {
					if (shot->IsBeam())
						sim->CreateExplosion(impact, GetVelocity(),	EExplosionType::BEAM_FLASH, 0.30f * scale, scale, region, this);
					else
						sim->CreateExplosion(impact, GetVelocity(), EExplosionType::HULL_FLASH, 0.30f * scale, scale, region, this);
				}

				FVector burst_vel = hull_impact - GetLocation();
				burst_vel.Normalize();
				burst_vel *= (float)(GetRadius() * 0.5);
				burst_vel += GetVelocity();

				if (sim)
					sim->CreateExplosion(hull_impact, burst_vel, EExplosionType::HULL_BURST, 0.50f * scale, scale, region, this);

				hit_type = HIT_HULL;
				hit_hull = true;
			}
		}
		else {
			if (dlen < GetRadius()) {
				hull_impact = impact = shot_loc;

				if (sim) {
					if (shot->IsBeam())
						sim->CreateExplosion(impact, GetVelocity(), EExplosionType::BEAM_FLASH, 0.30f * scale, scale, region, this);
					else
						sim->CreateExplosion(impact, GetVelocity(), EExplosionType::HULL_FLASH, 0.30f * scale, scale, region, this);
				}

				hit_type = HIT_HULL;
			}
		}
	}

	// DAMAGE RESOLUTION -------------------------------------------------

	if (hit_type != HIT_NOTHING) {
		double effective_damage = shot->Damage() * dscale;

		if (shot->IsBeam()) {
			effective_damage *= Game::FrameTime();
		}
		else {
			ApplyTorque(shot->GetVelocity() * (float)effective_damage * 1e-6f);
		}

		if (effective_damage > 0.0)
			Physical::InflictDamage(effective_damage);
	}

	return hit_type;
}

// +--------------------------------------------------------------------+

void
Debris::ExecFrame(double seconds)
{
	if (GetRegion()->GetType() == SimRegion::AIR_SPACE) {
		if (AltitudeAGL() < GetRadius()) {
			velocity = FVector::ZeroVector;
			arcade_velocity = FVector::ZeroVector;

			Terrain* terrain = region ? region->GetTerrain() : nullptr;

			if (terrain) {
				const FVector cur = GetLocation();
				MoveTo(FVector(cur.X, terrain->Height(cur.X, cur.Z), cur.Z));
			}
		}
		else {
			if (mass > 100.0f) {
				Orbital* primary = GetRegion()->GetOrbitalRegion()->Primary();

				const double m0 = primary ? primary->Mass() : 0.0;
				const double r = primary ? primary->Radius() : 1.0;

				SetDrag(0.001);
				SetGravity(6.0 * GRAV * m0 / (r * r));  // accentuate gravity
				SetBaseDensity(1.0f);
			}

			AeroFrame(seconds);
		}
	}
	else {
		Physical::ExecFrame(seconds);
	}
}

// +--------------------------------------------------------------------+

double
Debris::AltitudeAGL() const
{
	const FVector cur = GetLocation();
	double        altitude_agl = cur.Y;

	Terrain* terrain = region ? region->GetTerrain() : nullptr;

	if (terrain)
		altitude_agl -= terrain->Height(cur.X, cur.Z);

	if (!FMath::IsFinite(altitude_agl))
		altitude_agl = 0.0;

	return altitude_agl;
}
