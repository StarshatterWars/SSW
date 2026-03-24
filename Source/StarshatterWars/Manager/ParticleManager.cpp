/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         ParticleManager.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Particle Burst class (UE-compatible)
	- Keeps legacy API surface via ParticleManager (plain C++)
	- Legacy DirectX/Video/Sprite rendering is stubbed
	- Optional Niagara/Cascade spawning supported
*/

#include "ParticleManager.h"
#include "GameStructs.h"
#include "Game.h"

#include "Logging/LogMacros.h"
#include "HAL/PlatformMath.h"

#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterParticles, Log, All);

// +--------------------------------------------------------------------+
// Helpers

static inline float RandF()
{
	// Equivalent-ish to (rand()-16384)/32768 but using UE RNG:
	return FMath::FRandRange(-0.5f, 0.5f);
}

static FVector RandomVector(float magnitude)
{
	// Starshatter-style "random direction * magnitude"
	return FMath::VRand() * magnitude;
}

// +--------------------------------------------------------------------+
// ParticleManager (ported from Particles.cpp logic)

ParticleManager::ParticleManager(Bitmap* bitmap,
	int     np,
	const   FVector& base_loc,
	const   FVector& vel,
	float   bspeed,
	float   dr,
	float   s,
	float   bloom,
	float   dec,
	float   rate,
	bool    cont,
	bool    trail,
	bool    rise,
	int     a,
	int     nframes)
{
	(void)bitmap;
	(void)nframes;

	backend = SOFTWARE;

	nparts = FMath::Max(0, np);
	nverts = nparts;
	blend = a;

	continuous = cont;
	trailing = trail;
	rising = rise;
	emitting = true;

	base_speed = bspeed;
	max_speed = bspeed * 3.0f;
	drag = dr;
	min_scale = s;
	max_scale = bloom;
	decay = dec;
	release_rate = rate;
	extra = 0.0f;

	ref_loc = base_loc;
	offset = FVector::ZeroVector;

	velocity = nullptr;
	part_loc = nullptr;
	release = nullptr;
	intensity = nullptr;
	timestamp = nullptr;
	scale_arr = nullptr;
	angle = nullptr;
	frame = nullptr;

	point_sprite = nullptr;

	if (max_scale < min_scale)
		max_scale = min_scale;

	// Convert classic "decay > 2 => decay /= 256" behavior:
	if (decay > 2.0f)
		decay /= 256.0f;

	// Determine initial nverts behavior:
	if (nparts < 8) {
		nverts = 1;
	}
	else if (nparts > 50 || continuous) {
		nverts = (int)(nparts * 0.125f * release_rate);
		nverts = FMath::Clamp(nverts, 1, nparts);
	}

	// Allocate arrays:
	if (nparts > 0) {
		velocity = new FVector[nparts];
		part_loc = new FVector[nparts];
		release = new FVector[nparts];

		intensity = new float[nparts];
		timestamp = new float[nparts];
		scale_arr = new float[nparts];
		angle = new float[nparts];
		frame = new uint8[nparts];

		// Initialize the first "nverts" particles:
		float speed = base_speed;

		const float now_sec = (float)(Game::GameTime() / 1000.0);

		for (int i = 0; i < nverts; i++) {
			intensity[i] = 1.0f;
			timestamp[i] = now_sec;
			scale_arr[i] = min_scale;
			angle[i] = FMath::FRandRange(0.0f, 2.0f * PI);
			frame[i] = 0;

			part_loc[i] = FVector::ZeroVector;
			release[i] = ref_loc;

			velocity[i] = RandomVector(speed) + vel;

			if (speed < max_speed)
				speed += FMath::FRandRange(max_speed / 15.0f, max_speed / 5.0f);
			else
				speed = base_speed;
		}

		// If nverts < nparts, leave the rest uninitialized until emission expands.
		for (int i = nverts; i < nparts; i++) {
			intensity[i] = 0.0f;
			timestamp[i] = now_sec;
			scale_arr[i] = min_scale;
			angle[i] = 0.0f;
			frame[i] = 0;

			part_loc[i] = FVector::ZeroVector;
			release[i] = ref_loc;
			velocity[i] = FVector::ZeroVector;
		}
	}
}

ParticleManager::~ParticleManager()
{
	DestroyUE();

	delete[] velocity;
	delete[] part_loc;
	delete[] release;
	delete[] timestamp;
	delete[] intensity;
	delete[] scale_arr;
	delete[] angle;
	delete[] frame;

	velocity = nullptr;
	part_loc = nullptr;
	release = nullptr;
	timestamp = nullptr;
	intensity = nullptr;
	scale_arr = nullptr;
	angle = nullptr;
	frame = nullptr;

	point_sprite = nullptr;
}

// +--------------------------------------------------------------------+
// Unreal particle configuration/spawn

void ParticleManager::ConfigureNiagara(UNiagaraSystem* in_system)
{
	niagara_system = in_system;
	if (niagara_system) {
		backend = NIAGARA;
		if (cascade_comp)
			cascade_comp->DeactivateSystem();
	}
}

void ParticleManager::ConfigureCascade(UParticleSystem* in_system)
{
	cascade_system = in_system;
	if (cascade_system) {
		backend = CASCADE;
		if (niagara_comp)
			niagara_comp->Deactivate();
	}
}

bool ParticleManager::SpawnUE(UWorld* world, USceneComponent* attach_parent, const FVector& world_location)
{
	if (!world || !attach_parent) {
		UE_LOG(LogStarshatterParticles, Warning, TEXT("ParticleManager::SpawnUE failed: missing World or AttachParent."));
		return false;
	}

	DestroyUE();

	if (backend == NIAGARA) {
		if (!niagara_system) {
			UE_LOG(LogStarshatterParticles, Warning, TEXT("ParticleManager::SpawnUE NIAGARA failed: no Niagara system configured."));
			return false;
		}

		niagara_comp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			niagara_system,
			attach_parent,
			NAME_None,
			world_location,
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			true // bAutoDestroy
		);

		return niagara_comp != nullptr;
	}

	if (backend == CASCADE) {
		if (!cascade_system) {
			UE_LOG(LogStarshatterParticles, Warning, TEXT("ParticleManager::SpawnUE CASCADE failed: no Cascade system configured."));
			return false;
		}

		cascade_comp = UGameplayStatics::SpawnEmitterAttached(
			cascade_system,
			attach_parent,
			NAME_None,
			world_location,
			FRotator::ZeroRotator,
			EAttachLocation::KeepWorldPosition,
			true // bAutoDestroy
		);

		return cascade_comp != nullptr;
	}

	// SOFTWARE backend has no UE component:
	return false;
}

void ParticleManager::DestroyUE()
{
	if (niagara_comp) {
		niagara_comp->Deactivate();
		niagara_comp->DestroyComponent();
		niagara_comp = nullptr;
	}

	if (cascade_comp) {
		cascade_comp->DeactivateSystem();
		cascade_comp->DestroyComponent();
		cascade_comp = nullptr;
	}
}

void ParticleManager::SetWorldLocationUE(const FVector& world_location)
{
	if (niagara_comp)
		niagara_comp->SetWorldLocation(world_location);

	if (cascade_comp)
		cascade_comp->SetWorldLocation(world_location);
}

// +--------------------------------------------------------------------+
// Legacy API methods (UE-compatible)

void ParticleManager::ExecFrame(double seconds)
{
	// If using UE backend, we don't run software simulation:
	if (backend == NIAGARA || backend == CASCADE)
		return;

	if (!emitting || nparts < 1 || nverts < 1 || !velocity || !part_loc || !release || !intensity || !timestamp || !scale_arr || !angle || !frame)
		return;

	// ref_loc tracks the manager world location:
	// NOTE: In the original, this was Graphic::loc. Here we approximate by using ref_loc itself.
	// External code can set ref_loc directly if needed.
	const float dt = (float)seconds;
	if (dt <= 0.0f)
		return;

	// Radius behavior was used for frustum checks in the legacy projector system.
	// We keep the growth calculation for compatibility, but it is not used unless you wire CheckVisibility.
	// radius += max_speed * dt;  // (radius member is in Graphic; not assumed here)

	const float scaled_drag = FMath::Exp(-drag * dt);
	const float scale_inc = (max_scale - min_scale) * dt * 2.0f;

	for (int i = 0; i < nverts; i++) {
		part_loc[i] += velocity[i] * dt;

		if (rising) {
			// Original used y-up; Unreal uses Z-up.
			part_loc[i].Z += (RandF() + 1.0f) * scale_arr[i] * 80.0f * dt;
		}

		// Blooming:
		if (max_scale > 0.0f && scale_arr[i] < max_scale) {
			scale_arr[i] += scale_inc * ((float)(i % 3) / 3.0f);
		}

		// Rotation:
		double rho = angle[i];
		switch (i % 4) {
		case 0: rho += dt * 0.13; break;
		case 1: rho -= dt * 0.11; break;
		case 2: rho += dt * 0.09; break;
		case 3: rho -= dt * 0.07; break;
		default: break;
		}
		angle[i] = (float)rho;

		// Decay:
		intensity[i] -= decay * dt;

		// Legacy sprite frames were time-based. We keep frame=0 unless you wire sprite support back in.
		frame[i] = 0;

		// Drag:
		velocity[i] *= scaled_drag;
	}

	// Emit more particles over time if nverts < nparts:
	if (nverts < nparts && emitting) {
		const int   old_nverts = nverts;
		const double delta = (double)nparts * (double)release_rate * (double)seconds;
		const int   new_parts = (int)(delta + extra);

		extra = (float)(delta + extra - new_parts);
		nverts += new_parts;

		if (nverts > nparts)
			nverts = nparts;

		const float now_sec = (float)(Game::GameTime() / 1000.0f);

		for (int i = old_nverts; i < nverts; i++) {
			intensity[i] = 1.0f;
			timestamp[i] = now_sec;
			scale_arr[i] = min_scale;
			angle[i] = FMath::FRandRange(0.0f, 2.0f * PI);
			frame[i] = 0;

			part_loc[i] = FVector::ZeroVector;
			release[i] = ref_loc;
		}
	}

	if (nverts > nparts)
		nverts = nparts;

	// Recycle dead particles if continuous:
	if (continuous) {
		float speed = base_speed;
		const float now_sec = (float)(Game::GameTime() / 1000.0f);

		for (int i = 0; i < nverts; i++) {
			if (intensity[i] <= 0.0f) {
				part_loc[i] = FVector::ZeroVector;
				release[i] = ref_loc;

				intensity[i] = 1.0f;
				timestamp[i] = now_sec;
				scale_arr[i] = min_scale;
				angle[i] = PI * FMath::FRand(); // approx
				frame[i] = 0;

				velocity[i] = RandomVector(speed);

				if (speed < max_speed)
					speed += FMath::FRandRange(max_speed / 25.0f, max_speed / 18.0f);
				else
					speed = base_speed;
			}
		}
	}
}

bool ParticleManager::CheckVisibility(SimProjector& projector)
{
	(void)projector;

	// Legacy Projector/Video stack removed for UE.
	// Treat as visible; UE visibility should be handled by component culling.
	return true;
}

void ParticleManager::Render(Video* video, uint32 flags)
{
	(void)video;
	(void)flags;

	// Legacy rendering removed for UE compatibility.
	// Use Niagara/Cascade backend, or re-implement using your UE HUD/Canvas path.
}
