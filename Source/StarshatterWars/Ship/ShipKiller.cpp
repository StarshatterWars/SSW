/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars
	FILE:         ShipKiller.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ShipKiller class implementation
*/

#include "ShipKiller.h"

#include "Sim.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimSystem.h"
#include "Weapon.h"
#include "SimShot.h"
#include "Explosion.h"
#include "Debris.h"
#include "HUDSounds.h"

#include "Solid.h"
#include "Random.h"

// Unreal logging:
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipKiller, Log, All);

// +----------------------------------------------------------------------+

ShipKiller::ShipKiller(Ship* s)
	: ship(s),
	time(0.0f),
	loc(FVector::ZeroVector),
	exp_time(0.0f),
	exp_index(0)
{
}

ShipKiller::~ShipKiller()
{
}

// +----------------------------------------------------------------------+

static inline int random_index()
{
	// Match legacy behavior: rand() in [0..32767] -> [0..9]
	return static_cast<int>(rand() / 3277);
}

// +----------------------------------------------------------------------+

void ShipKiller::BeginDeathSpiral()
{
	if (!ship)
		return;

	// shut down all ship systems:
	ListIter<SimSystem> iter = ship->GetSystems();
	while (++iter) {
		iter->PowerOff();
		iter->SetPowerLevel(0);

		if (iter->GetType() == SYSTEM_CATEGORY::WEAPON) {
			Weapon* gun = (Weapon*)iter.value();

			for (int i = 0; i < Weapon::MAX_BARRELS; i++) {
				SimShot* beam = gun->GetBeam(i);
				if (beam)
					beam->Destroy();
			}
		}
	}

	if (ship->GetShieldRep())
		ship->GetShieldRep()->Hide();

	Sim* sim = Sim::GetSim();
	const ShipDesign* design = ship->Design();
	if (!design || !sim)
		return;

	const float time_to_go = (float)design->death_spiral_time;
	time = DEATH_CAM_LINGER + time_to_go;

	loc = ship->Location() + ship->Velocity() * (time_to_go - 1.0f);

	// Unreal: prefer FMath over rand()
	if (FMath::Rand() < 16000)
		loc += ship->BeamLine() * (-3.0f * (float)ship->Radius());
	else
		loc += ship->BeamLine() * (3.0f * (float)ship->Radius());

	if (FMath::Rand() < 8000)
		loc += ship->LiftLine() * (-1.0f * (float)ship->Radius());
	else
		loc += ship->LiftLine() * (2.0f * (float)ship->Radius());

	// stop on crash:
	if (ship->IsGroundUnit() || (ship->IsAirborne() && ship->AltitudeAGL() < ship->Radius() * 2)) {
		time = DEATH_CAM_LINGER;
		loc = ship->Location() + FVector(6.0f * (float)ship->Radius(),
			7.0f * (float)ship->Radius(),
			8.0f * (float)ship->Radius());
		ship->SetVelocity(FVector::ZeroVector);
	}
	// else, slow tumble:
	else {
		const FVector torque = RandomVector(ship->Mass() / 7);
		ship->ApplyTorque(torque);

		for (int i = 0; i < 5; i++) {
			exp_index = random_index() % ShipDesign::MAX_EXPLOSIONS;
			if (design->explosion[exp_index].type > 0 && !design->explosion[exp_index].final)
				break;
		}

		float exp_scale = (float)design->explosion_scale;
		if (exp_scale <= 0)
			exp_scale = (float)design->scale;

		exp_time = (float)design->explosion[exp_index].time;

		if (design->explosion[exp_index].type > 0) {
			// UE FIX: FVector * Matrix is not defined. Convert Starshatter Matrix rows to dot products.
			const Matrix& M = ship->Cam().Orientation();

			const FVector Row0((float)M(0, 0), (float)M(0, 1), (float)M(0, 2));
			const FVector Row1((float)M(1, 0), (float)M(1, 1), (float)M(1, 2));
			const FVector Row2((float)M(2, 0), (float)M(2, 1), (float)M(2, 2));

			const FVector Local = design->explosion[exp_index].loc;

			// Starshatter semantics: (Local * Orientation) => components are dot(Local, row)
			const FVector Rotated(
				FVector::DotProduct(Local, Row0),
				FVector::DotProduct(Local, Row1),
				FVector::DotProduct(Local, Row2)
			);

			const FVector exp_loc = ship->Location() + Rotated;

			sim->CreateExplosion(
				exp_loc,
				ship->Velocity(),
				design->explosion[exp_index].type,
				(float)ship->Radius(),
				exp_scale,
				ship->GetRegion(),
				ship
			);
		}
	}

	ship->SetControls(0);
	ship->SetupAgility();

	UE_LOG(LogShipKiller, Verbose, TEXT("BeginDeathSpiral: ship=%p time=%.2f exp_index=%d"), ship, time, exp_index);
}

// +----------------------------------------------------------------------+

void ShipKiller::ExecFrame(double seconds)
{
	Sim* sim = Sim::GetSim();
	const ShipDesign* design = ship ? ship->Design() : nullptr;

	if (!ship || !sim || !design) {
		UE_LOG(LogShipKiller, Warning, TEXT("ExecFrame: invalid state ship=%p sim=%p design=%p"), ship, sim, design);
		return;
	}

	// UE COMPAT: replace "FVector * Matrix" with an explicit rotation transform.
	// Assumption: ship->Cam().Orientation() returns your Starshatter Matrix type.
	// This lambda converts Matrix -> FRotationMatrix and transforms a direction vector.
	const auto TransformDirByShipCam = [&](const FVector& LocalDir) -> FVector
		{
			const Matrix& M = ship->Cam().Orientation();

			// Convert Starshatter Matrix basis into an Unreal FMatrix.
			// The original code uses rows as basis vectors:
			//   x' = v dot row0, y' = v dot row1, z' = v dot row2
			// Build UE basis vectors accordingly.
			const FVector Row0((float)M(0, 0), (float)M(0, 1), (float)M(0, 2));
			const FVector Row1((float)M(1, 0), (float)M(1, 1), (float)M(1, 2));
			const FVector Row2((float)M(2, 0), (float)M(2, 1), (float)M(2, 2));

			// Apply "vector * matrix" semantics (dot with rows):
			return FVector(
				(float)(LocalDir | Row0),
				(float)(LocalDir | Row1),
				(float)(LocalDir | Row2)
			);
		};

	time -= (float)seconds;
	exp_time -= (float)seconds;

	float exp_scale = design->explosion_scale;
	if (exp_scale <= 0)
		exp_scale = design->scale;

	if (exp_time < 0) {
		exp_index++;
		if (exp_index >= ShipDesign::MAX_EXPLOSIONS || design->explosion[exp_index].final)
			exp_index = 0;

		exp_time = design->explosion[exp_index].time;

		if (design->explosion[exp_index].type > 0) {
			const FVector exp_loc =
				ship->Location() + TransformDirByShipCam(design->explosion[exp_index].loc);

			sim->CreateExplosion(exp_loc,
				ship->Velocity(),
				design->explosion[exp_index].type,
				(float)ship->Radius(),
				exp_scale,
				ship->GetRegion(),
				ship);
		}
	}

	if (time < DEATH_CAM_LINGER) {
		for (int i = 0; i < ShipDesign::MAX_EXPLOSIONS; i++) {
			if (design->explosion[i].final) {
				const FVector exp_loc =
					ship->Location() + TransformDirByShipCam(design->explosion[i].loc);

				sim->CreateExplosion(exp_loc,
					ship->Velocity(),
					design->explosion[i].type,
					(float)ship->Radius(),
					exp_scale,
					ship->GetRegion());
			}
		}

		for (int i = 0; i < ShipDesign::MAX_DEBRIS; i++) {
			if (design->debris[i].model) {
				const FVector debris_loc =
					ship->Location() + TransformDirByShipCam(design->debris[i].loc);

				FVector debris_vel = debris_loc - ship->Location();
				debris_vel.Normalize();

				if (design->debris[i].speed > 0)
					debris_vel *= design->debris[i].speed;
				else
					debris_vel *= 200.0f;

				if (ship->IsGroundUnit()) {
					debris_vel *= 2.0f;

					// keep debris from going underground:
					if (debris_vel.Y < 0)
						debris_vel.Y *= -1.0f;
				}

				for (int n = 0; n < design->debris[i].count; n++) {
					Debris* debris = sim->CreateDebris(debris_loc,
						debris_vel + ship->Velocity(),
						design->debris[i].model,
						design->debris[i].mass,
						ship->GetRegion());

					debris->SetLife(design->debris[i].life);
					debris->SetDrag(design->debris[i].drag);

					if (n == 0) {
						debris->CloneCam(ship->Cam());
						debris->MoveTo(debris_loc);
					}

					for (int fire = 0; fire < 5; fire++) {
						if (design->debris[i].fire_loc[fire] == FVector::ZeroVector)
							continue;

						const FVector fire_loc =
							debris->Location() + TransformDirByShipCam(design->debris[i].fire_loc[fire]);

						if (design->debris[i].fire_type > 0) {
							sim->CreateExplosion(fire_loc,
								ship->Velocity(),
								design->debris[i].fire_type,
								exp_scale,
								exp_scale,
								ship->GetRegion(),
								debris);
						}
						else {
							sim->CreateExplosion(fire_loc,
								ship->Velocity(),
								Explosion::SMALL_FIRE,
								exp_scale,
								exp_scale,
								ship->GetRegion(),
								debris);

							sim->CreateExplosion(fire_loc,
								ship->Velocity(),
								Explosion::SMOKE_TRAIL,
								exp_scale * 0.25f,
								exp_scale * 0.25f,
								ship->GetRegion(),
								debris);
						}
					}

					if (n + 1 < design->debris[i].count) {
						debris_vel = RandomVector(1);

						if (design->debris[i].speed > 0)
							debris_vel *= static_cast<float>(design->debris[i].speed) * FMath::FRandRange(0.8f, 1.2f);
						else
							debris_vel *= (300.0 + rand() / 50.0);
					}
				}
			}
		}

		if (ship == sim->GetPlayerShip())
			HUDSounds::StopSound(HUDSounds::SND_RED_ALERT);

		sim->CreateSplashDamage(ship);

		// CAREFUL!!! This will also delete this object!
		ship->Destroy();
	}
}
