/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         ShipManager.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Starship (or space/ground station) controller/director class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "MotionController.h"
#include "SimDirector.h"

// +--------------------------------------------------------------------+

class Ship;
class ShipDesign;
class KeyMap;

// +--------------------------------------------------------------------+

class ShipManager : public SimDirector
{
public:
	enum TYPE { DIR_TYPE = 1 };

	ShipManager(Ship* s, MotionController* m);

	virtual void      ExecFrame(double seconds);
	virtual int       Subframe()  const { return true; }
	virtual void      Launch();

	static  int       KeyDown(int action);
	static  int       Toggled(int action);

	virtual int       Type()      const { return DIR_TYPE; }

protected:
	Ship* ship;
	MotionController* controller;

	bool              throttle_active;
	bool              launch_latch;
	bool              pickle_latch;
	bool              target_latch;
};
