/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         FlightPlanner.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Flight Planning class for creating navpoint routes for fighter elements.
	Used both by the CarrierAI class and the Flight Dialog.
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

// +--------------------------------------------------------------------+

class FlightPlanner
{
public:
	FlightPlanner(Ship* s);
	virtual ~FlightPlanner();

	virtual void      CreatePatrolRoute(SimElement* elem, int index);
	virtual void      CreateStrikeRoute(SimElement* strike, SimElement* target);
	virtual void      CreateEscortRoute(SimElement* escort, SimElement* ward);

	Sim*			  sim;
	Ship*			  ship;
	float             patrol_range;
};

// +--------------------------------------------------------------------+
