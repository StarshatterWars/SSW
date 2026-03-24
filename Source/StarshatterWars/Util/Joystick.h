/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         Joystick.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Joystick Input class (Unreal-native polling)

	- Plain C++ class for Starshatter compatibility.
	- Polls Unreal input state indirectly via UJoystickManager (UGameInstanceSubsystem).
	- Maintains legacy Starshatter API surface where required.
*/

#pragma once

#include "MotionController.h"

// Forward declarations (keep header light; no Unreal includes here):
class UObject;
class UJoystickManager;

class Joystick : public MotionController
{
public:
	static const char* TYPENAME() { return "Joystick"; }

	Joystick();
	virtual ~Joystick();

	// setup
	virtual void   MapKeys(KeyMapEntry* mapping, int nkeys) override;

	// sample the physical device (Unreal polling via JoystickManager)
	virtual void   Acquire() override;

	// translations
	virtual double X() override { return x; }
	virtual double Y() override { return y; }
	virtual double Z() override { return z; }

	// rotations
	virtual double Pitch() override { return p; }
	virtual double Roll()  override { return r; }
	virtual double Yaw()   override { return w; }
	virtual int    Center() override { return 0; }

	// throttle
	virtual double Throttle() override { return t; }
	virtual void SetThrottle(double newThrottle) override { t = newThrottle; }

	// actions
	virtual int    Action(int n) override { return (n >= 0 && n < MotionController::MaxActions) ? (action[n] ? 1 : 0) : 0; }
	virtual int    ActionMap(int n) override { return KeyDownMap(n) ? 1 : 0; }

	static bool    KeyDown(int key);
	static bool    KeyDownMap(int key);

	static Joystick* GetInstance();

	// Unreal does not expose the old DI/MM device list the same way; keep these for API stability.
	// No-op / stub implementations in Joystick.cpp.
	static void        EnumerateDevices();
	static int         NumDevices();
	static const char* GetDeviceName(int i);

	// Optional raw axis accessor for debugging; derived from last polled state.
	static int         ReadRawAxis(int axis);

	static int         GetAxisMap(int n);
	static int         GetAxisInv(int n);

protected:
	void           ProcessAxes(double joy_x, double joy_y, double joy_r, double joy_t);
	void           ResetHat();

	double         x = 0.0;
	double         y = 0.0;
	double         z = 0.0;
	double         p = 0.0;
	double         r = 0.0;
	double         w = 0.0;
	double         t = 0.0;

	bool           action[MotionController::MaxActions] = { false };
	bool           hat[4][4] = { { false } };

	int            map[KEY_MAP_SIZE] = { 0 };
	int            map_axis[4] = { 0 };
	bool           inv_axis[4] = { false };

private:
	// Helper: find a usable WorldContext without forcing Starshatter core to become UObject-aware.
	static const UObject* FindWorldContextObject();
};
