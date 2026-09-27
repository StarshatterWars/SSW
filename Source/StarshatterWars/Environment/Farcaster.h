/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Farcaster.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "SimObject.h"
#include "Text.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"

// +----------------------------------------------------------------------+

class Ship;
class ShipDesign;
class Physical;

// +----------------------------------------------------------------------+

class Farcaster : public SimSystem, public SimObserver
{
public:
	Farcaster(double capacity, double sink_rate);
	Farcaster(const Farcaster& rhs);
	virtual ~Farcaster();

	enum CONSTANTS { NUM_APPROACH_PTS = 4 };

	virtual void   ExecFrame(double seconds);
	void           SetShip(Ship* s) { ship = s; }
	void           SetDest(Ship* d) { dest = d; }

	FVector        ApproachPoint(int i) const { return approach_point[i]; }
	FVector        StartPoint()         const { return start_point; }
	FVector        EndPoint()           const { return end_point; }

	virtual void   SetApproachPoint(int i, FVector loc);
	virtual void   SetStartPoint(FVector loc);
	virtual void   SetEndPoint(FVector loc);
	virtual void   SetCycleTime(double time);

	virtual void   Orient(const Physical* rep);

	// SimObserver:
	virtual bool         Update(SimObject* obj);
	virtual const char* GetObserverName() const;

	// accessors:
	const Ship* GetShip()      const { return ship; }
	const Ship* GetDest()      const { return dest; }

	int            ActiveState()  const { return active_state; }
	double         WarpFactor()   const { return warp_fov; }

protected:
	virtual void      Jump();
	virtual void      Arrive(Ship* s);

	Ship* ship;
	Ship* dest;
	Ship* jumpship;

	FVector           start_rel;
	FVector           end_rel;
	FVector           approach_rel[NUM_APPROACH_PTS];

	FVector           start_point;
	FVector           end_point;
	FVector           approach_point[NUM_APPROACH_PTS];

	double            cycle_time;
	int               active_state;
	double            warp_fov;

	bool              no_dest;
};
