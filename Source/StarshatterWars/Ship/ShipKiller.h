/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShipKiller.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ShipKiller (i.e. death spiral) class
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Text.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class ShipKiller
{
public:
	static constexpr float DEATH_CAM_LINGER = 2.0f;

	// CONSTRUCTORS:
	ShipKiller(Ship* inShip);
	virtual ~ShipKiller();

	virtual void      BeginDeathSpiral();
	virtual void      ExecFrame(double seconds);

	// GENERAL ACCESSORS:
	virtual float     TransitionTime() const { return time; }
	virtual FVector   TransitionLoc()  const { return loc; }

protected:
	Ship* ship = nullptr;

	float             time = 0.0f;
	FVector           loc = FVector::ZeroVector;

	float             exp_time = 0.0f;
	int               exp_index = 0;
};
