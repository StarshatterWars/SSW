/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         MultiController.cpp
	AUTHOR:       John DiCamillo

	UNREAL PORT:
	- Removes MemDebug
	- Keeps original behavior and member variable names EXACTLY
	- Uses nullptr and UE FORCEINLINE where appropriate
*/

#include "MultiController.h"
#include "CoreMinimal.h"

// +--------------------------------------------------------------------+

MultiController::MultiController()
	: nctrl(0)
	, x(0.0), y(0.0), z(0.0)
	, p(0.0), r(0.0), w(0.0)
	, t(0.0)
	, p1(0.0), r1(0.0), w1(0.0)
	, c(0)
{
	for (int i = 0; i < MotionController::MaxActions; i++)
		action[i] = 0;

	for (int i = 0; i < 4; i++)
		ctrl[i] = nullptr;
}

MultiController::~MultiController()
{
	for (int i = 0; i < 4; i++)
	{
		delete ctrl[i];
		ctrl[i] = nullptr;
	}
}

// +--------------------------------------------------------------------+

void MultiController::AddController(MotionController* mc)
{
	if (nctrl < 4 && mc)
		ctrl[nctrl++] = mc;
}

void MultiController::MapKeys(KeyMapEntry* mapping, int nkeys)
{
	for (int i = 0; i < nctrl; i++)
	{
		if (ctrl[i])
			ctrl[i]->MapKeys(mapping, nkeys);
	}
}

int MultiController::GetSwapYawRoll() const
{
	// Base class version is non-const, but ctrl[] pointers are non-const.
	// This preserves original behavior while keeping this method const.
	if (nctrl && ctrl[0])
		return ctrl[0]->GetSwapYawRoll();

	return 0;
}

void MultiController::SwapYawRoll(int swap)
{
	for (int i = 0; i < nctrl; i++)
	{
		if (ctrl[i])
			ctrl[i]->SwapYawRoll(swap);
	}
}

// +--------------------------------------------------------------------+

static FORCEINLINE void ClampUnit(double& v)
{
	if (v < -1.0) v = -1.0;
	else if (v > 1.0) v = 1.0;
}

void MultiController::Acquire()
{
	// legacy reset: t = x = y = z = p = r = w = c = 0;
	t = 0.0;
	x = 0.0;
	y = 0.0;
	z = 0.0;
	p = 0.0;
	r = 0.0;
	w = 0.0;
	c = 0;

	for (int i = 0; i < MotionController::MaxActions; i++)
		action[i] = 0;

	for (int i = 0; i < nctrl; i++)
	{
		MotionController* mc = ctrl[i];
		if (!mc)
			continue;

		mc->Acquire();

		x += mc->X();
		y += mc->Y();
		z += mc->Z();

		r += mc->Roll();
		p += mc->Pitch();
		w += mc->Yaw();

		c += mc->Center();
		t += mc->Throttle();

		for (int a = 0; a < MotionController::MaxActions; a++)
			action[a] += mc->Action(a);
	}

	ClampUnit(x);
	ClampUnit(y);
	ClampUnit(z);
	ClampUnit(r);
	ClampUnit(p);
	ClampUnit(w);
	ClampUnit(t);
}

// +--------------------------------------------------------------------+

void MultiController::SetThrottle(double Throttle)
{
	for (int i = 0; i < nctrl; i++)
	{
		if (ctrl[i])
			ctrl[i]->SetThrottle(Throttle);
	}
}

// +--------------------------------------------------------------------+

int MultiController::ActionMap(int key)
{
	for (int i = 0; i < nctrl; i++)
	{
		MotionController* mc = ctrl[i];
		if (!mc)
			continue;

		int result = mc->ActionMap(key);
		if (result)
			return result;
	}

	return 0;
}
