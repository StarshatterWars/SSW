/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         ParticleManager.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Particle burst class (supports legacy software particles and optional Unreal Niagara/Cascade)
*/

#pragma once

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

#include "Types.h"
#include "Geometry.h"
#include "Graphic.h"
#include "Sprite.h"

// +--------------------------------------------------------------------+
// Forward declarations (keep header light):

class Bitmap;
class Video;
class SimProjector;

class UWorld;
class UObject;
class USceneComponent;
class AActor;

class UParticleSystem;
class UParticleSystemComponent;

class UNiagaraSystem;
class UNiagaraComponent;

// +--------------------------------------------------------------------+

class ParticleManager : public Graphic
{
public:
	enum ParticleBackend
	{
		SOFTWARE = 0,   // classic Starshatter-style point sprite particles
		CASCADE = 1,   // UParticleSystem / UParticleSystemComponent (legacy UE)
		NIAGARA = 2    // UNiagaraSystem / UNiagaraComponent (UE5 preferred)
	};

public:
	ParticleManager(Bitmap* bitmap,
		int     np,
		const   FVector& base_loc,
		const   FVector& vel,
		float   base_speed = 500.0f,
		float   drag = 1.0f,
		float   scale = 1.0f,
		float   bloom = 0.0f,
		float   decay = 100.0f,
		float   release = 1.0f,
		bool    cont = false,
		bool    trail = true,
		bool    rise = false,
		int     blend = 3,
		int     nframes = 1);

	virtual ~ParticleManager();

	// ------------------------------------------------------------
	// Classic software particles (kept for compatibility):
	virtual void   Render(Video* video, uint32 flags);
	virtual void   ExecFrame(double seconds);
	virtual void   TranslateBy(const FVector& ref) { offset = ref; loc = loc - ref; }
	virtual bool   CheckVisibility(SimProjector& projector);

	virtual bool   IsEmitting()   const { return emitting; }
	virtual void   StopEmitting() { emitting = false; }

	// ------------------------------------------------------------
	// Unreal particle integration (optional):
	// - This manager stays a plain C++ class; it does not own UObject lifetimes.
	// - If you use UE particles, call Configure* then SpawnUE() with a valid attach parent.
	// - DestroyUE() will deactivate and release component pointers (does not delete assets).

	void           SetBackend(ParticleBackend in_backend) { backend = in_backend; }
	ParticleBackend GetBackend() const { return backend; }

	void           ConfigureNiagara(UNiagaraSystem* in_system);
	void           ConfigureCascade(UParticleSystem* in_system);

	// Spawn a component attached to AttachParent. World is required to create components safely.
	// Returns true if a UE component is active after the call.
	bool           SpawnUE(UWorld* world, USceneComponent* attach_parent, const FVector& world_location);
	void           DestroyUE();

	// Update the UE component transform (if active).
	void           SetWorldLocationUE(const FVector& world_location);

	// Allow external code to access active components (optional convenience).
	UNiagaraComponent* GetNiagaraComponent() const { return niagara_comp; }
	UParticleSystemComponent* GetCascadeComponent() const { return cascade_comp; }

protected:
	// ------------------------------------------------------------
	// Software particle state:
	void           ProcessAxes(double joy_x, double joy_y, double joy_r, double joy_t); // (unused here; kept if you share utilities)

	int         nparts = 0;
	int         nverts = 0;
	int         blend = 0;

	bool        continuous = false;
	bool        trailing = false;
	bool        rising = false;
	bool        emitting = false;

	float       base_speed = 0.0f;
	float       max_speed = 0.0f;
	float       drag = 0.0f;
	float       release_rate = 0.0f;
	float       decay = 0.0f;
	float       min_scale = 0.0f;
	float       max_scale = 0.0f;
	float       extra = 0.0f;

	FVector     ref_loc = FVector::ZeroVector;
	FVector     offset = FVector::ZeroVector;

	FVector* velocity = nullptr;
	FVector* part_loc = nullptr;
	FVector* release = nullptr;

	float* timestamp = nullptr;
	float* intensity = nullptr;
	float* scale_arr = nullptr;
	float* angle = nullptr;

	uint8* frame = nullptr;

	Sprite* point_sprite = nullptr;

	// ------------------------------------------------------------
	// Unreal particle state:
	ParticleBackend backend = SOFTWARE;

	// Templates (assets) provided by external code; not owned:
	UParticleSystem* cascade_system = nullptr;
	UNiagaraSystem* niagara_system = nullptr;

	// Spawned components; owned by Unreal world/actor hierarchy (not by this class):
	UParticleSystemComponent* cascade_comp = nullptr;
	UNiagaraComponent* niagara_comp = nullptr;
};
