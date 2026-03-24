/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         CarrierAI.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	"Air Boss" AI class for managing carrier fighter squadrons
*/

#pragma once

#include "Types.h"
#include "SimDirector.h"

// +--------------------------------------------------------------------+

class Sim;
class Ship;
class ShipAI;
class Instruction;
class Hangar;
class SimElement;
class FlightPlanner;

// +--------------------------------------------------------------------+

class CarrierAI : public SimDirector
{
public:
	CarrierAI(Ship* s, int level);
	virtual ~CarrierAI();

	virtual void		ExecFrame(double seconds);

protected:
	virtual bool		CheckPatrolCoverage();
	virtual bool		CheckHostileElements();

	virtual bool		CreateStrike(SimElement* elem);

	virtual SimElement* CreatePackage(int squad, int size, int code, const char* target = 0, const char* loadname = 0);
	virtual bool		LaunchElement(SimElement* elem);

	Sim* sim;
	Ship* ship;
	Hangar* hangar;
	FlightPlanner* flight_planner;
	int					exec_time;
	int					hold_time;
	int					ai_level;

	SimElement* patrol_elem[4];
};
