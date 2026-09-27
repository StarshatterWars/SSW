/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         MouseController.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Mouse Input class (MotionController)
*/

#pragma once

#include "MotionController.h"

// +--------------------------------------------------------------------+

class MouseController : public MotionController
{
public:
	static const char* TYPENAME() { return "MouseController"; }

	MouseController();
	virtual ~MouseController();

	// setup:
	virtual void   MapKeys(KeyMapEntry* mapping, int nkeys);

	// sample the physical device:
	virtual void   Acquire();

	// translations:
	virtual double X() { return 0; }
	virtual double Y() { return 0; }
	virtual double Z() { return 0; }

	// rotations:
	virtual double Pitch() { return active ? p : 0; }
	virtual double Roll() { return active ? r : 0; }
	virtual double Yaw() { return active ? w : 0; }
	virtual int    Center() { return 0; }

	// throttle:
	virtual double Throttle() { return active ? t : 0; }
	virtual void   SetThrottle(double inThrottle) { t = inThrottle; } // avoid hiding MotionController::throttle

	// actions:
	virtual int    Action(int n) { return action[n]; }
	virtual int    ActionMap(int n);

	// actively sampling?
	virtual bool   Active() { return active; }
	virtual void   SetActive(bool a) { active = a; }

	static MouseController* GetInstance();

protected:
	double         p, r, w, dx, dy, t;
	int            action[MotionController::MaxActions];
	int            map[32];
	bool           active;
	int            active_key;
};
