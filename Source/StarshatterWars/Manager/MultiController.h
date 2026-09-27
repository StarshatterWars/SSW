/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         MultiController.h
	AUTHOR:       John DiCamillo

	UNREAL PORT:
	- Uses #pragma once
	- Uses nullptr
	- Keeps original member variable names EXACTLY (nctrl, ctrl, x,y,z,p,r,w,t,p1,r1,w1,c,action)
	- Keeps base class signatures (double/int) so overrides match MotionController.h as provided
*/

#pragma once

#include "CoreMinimal.h"
#include "MotionController.h"

// +--------------------------------------------------------------------+

class MultiController : public MotionController
{
public:
	static const char* TYPENAME() { return "MultiController"; }

	MultiController();
	virtual ~MultiController() override;

	virtual void   AddController(MotionController* c);
	virtual void   MapKeys(KeyMapEntry* mapping, int nkeys) override;

	// NOTE: base GetSwapYawRoll() is non-const, but original MultiController exposed const.
	// We keep the original signature here (const) without "override".
	virtual int    GetSwapYawRoll() const;
	virtual void   SwapYawRoll(int swap) override;

	// sample the physical device:
	virtual void   Acquire() override;

	// translations:
	virtual double X() override { return x; }
	virtual double Y() override { return y; }
	virtual double Z() override { return z; }

	// rotations:
	virtual double Pitch() override { return p; }
	virtual double Roll() override { return r; }
	virtual double Yaw() override { return w; }
	virtual int    Center() override { return c; }

	// throttle:
	virtual double Throttle() override { return t; }
	virtual void   SetThrottle(double throttle) override;

	// actions:
	virtual int    Action(int n) override { return action[n]; }
	virtual int    ActionMap(int n) override;

protected:
	int               nctrl;
	MotionController* ctrl[4];

	double            x, y, z;
	double            p, r, w;
	double            t;

	// retained for parity with original header
	double            p1, r1, w1;

	int               c;

	int               action[MotionController::MaxActions];
};
