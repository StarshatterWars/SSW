/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Shield.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Conventional Shield (system) class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "GameStructs_System.h"

// No FVector/Geometry usage required in this header; keep Unreal includes out.

// +--------------------------------------------------------------------+

class SimShot;
class USound;

// +--------------------------------------------------------------------+

class Shield : public SimSystem
{
public:

	Shield(EShieldType s);
	Shield(const Shield& rhs);
	virtual ~Shield();

	virtual void   ExecFrame(double seconds);
	double         DeflectDamage(SimShot* shot, double shot_damage);

	double         GetShieldLevel()              const { return shield_level * 100; }
	double         GetShieldFactor()             const { return shield_factor; }

	double         GetShieldCurve()				const { return shield_curve; }
	void           SetShieldFactor(double f)	{ shield_factor = (float)f; }
	
	void           SetShieldCurve(double c)		{ shield_curve = (float)c; }

	double         GetShieldCutoff()             const { return shield_cutoff; }
	void           SetShieldCutoff(double f)	 { shield_cutoff = (float)f; }

	double         GetCapacity()              const { return capacity; }

	double         GetConsumption()				const { return sink_rate; }
	void           SetConsumption(double r)		{ sink_rate = (float)r; }

	bool           GetShieldCapacitor()       const { return shield_capacitor; }
	void           SetShieldCapacitor(bool c);

	bool           GetShieldBubble()			const { return shield_bubble; }
	void           SetShieldBubble(bool b)		{ shield_bubble = b; }

	double         GetDeflectionCost()			const { return deflection_cost; }
	void           SetDeflectionCost(double c) { deflection_cost = (float)c; }

	// override from System:
	virtual void   SetPowerLevel(double level);
	virtual void   SetNetShieldLevel(int level);

	virtual void   Distribute(double delivered_energy, double seconds);
	virtual void   DoEMCON(int emcon);

protected:
	bool           shield_capacitor;
	bool           shield_bubble;
	float          shield_factor;
	float          shield_level;
	float          shield_curve;
	float          shield_cutoff;
	float          requested_power_level;
	float          deflection_cost;
};
