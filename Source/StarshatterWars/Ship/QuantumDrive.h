/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         QuantumDrive.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Quantum (JUMP) Drive (system) class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "Geometry.h"

// Minimal Unreal include (required by request: convert Point/Vec3 to FVector):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Ship;
class SimRegion;

// +--------------------------------------------------------------------+

class QuantumDrive : public SimSystem
{
public:
	enum SUBTYPE { QUANTUM, HYPER };

	QuantumDrive(SUBTYPE s, double capacity, double sink_rate);
	QuantumDrive(const QuantumDrive& rhs);
	virtual ~QuantumDrive();

	enum ACTIVE_STATES
	{
		ACTIVE_READY,
		ACTIVE_COUNTDOWN,
		ACTIVE_PREWARP,
		ACTIVE_POSTWARP
	};

	void              SetDestination(SimRegion* rgn, const FVector& loc);
	bool              Engage(bool immediate = false);
	int               ActiveState() const { return active_state; }
	double            WarpFactor()  const { return warp_fov; }
	double            JumpTime()    const { return jump_time; }
	virtual void      PowerOff();

	virtual void      ExecFrame(double seconds);

	void              SetShip(Ship* s) { ship = s; }
	Ship*			  GetShip() const { return ship; }

	double            GetCountdown() const { return countdown; }
	void              SetCountdown(double d) { countdown = d; }

protected:
	void				Jump();
	void				AbortJump();

	int					active_state;

	Ship*				ship;
	double				warp_fov;
	double				jump_time;
	double				countdown;

	SimRegion*			dst_rgn;
	FVector             dst_loc;
};
