/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         MotionController.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Abstract MotionController class (hides details of Joystick, Keyboard, etc.)
*/

#pragma once

// +--------------------------------------------------------------------+

struct KeyMapEntry
{
	static const char* TYPENAME() { return "KeyMapEntry"; }

	KeyMapEntry()
		: act(0)
		, key(0)
		, alt(0)
		, joy(0)
	{
	}

	KeyMapEntry(int32 InAct, int32 InKey, int32 InAlt = 0, int32 InJoy = 0)
		: act(InAct)
		, key(InKey)
		, alt(InAlt)
		, joy(InJoy)
	{
	}

	bool operator==(const KeyMapEntry& Other) const
	{
		return act == Other.act &&
			key == Other.key &&
			alt == Other.alt &&
			joy == Other.joy;
	}

	bool operator!=(const KeyMapEntry& Other) const
	{
		return !(*this == Other);
	}

	int32 act;
	int32 key;
	int32 alt;
	int32 joy;
};


// +--------------------------------------------------------------------+

inline constexpr int KEY_MAP_SIZE = 256;
inline constexpr int KEY_BASE_SIZE = 64;
inline constexpr int KEY_USER_SIZE = KEY_MAP_SIZE - KEY_BASE_SIZE;

inline constexpr int KEY_MAP_BASE = 0;
inline constexpr int KEY_MAP_END = KEY_MAP_BASE + KEY_BASE_SIZE - 1;

inline constexpr int KEY_USER_BASE = KEY_MAP_END + 1;
inline constexpr int KEY_USER_END = KEY_USER_BASE + KEY_USER_SIZE - 1;

inline constexpr int KEY_MAP_FIRST = KEY_MAP_BASE;
inline constexpr int KEY_MAP_LAST = KEY_MAP_BASE + KEY_MAP_SIZE - 1;

// MAP NAMES:

inline constexpr int KEY_PLUS_X = 1;
inline constexpr int KEY_MINUS_X = 2;
inline constexpr int KEY_PLUS_Y = 3;
inline constexpr int KEY_MINUS_Y = 4;
inline constexpr int KEY_PLUS_Z = 5;
inline constexpr int KEY_MINUS_Z = 6;

inline constexpr int KEY_PITCH_UP = 7;
inline constexpr int KEY_PITCH_DOWN = 8;
inline constexpr int KEY_YAW_LEFT = 9;
inline constexpr int KEY_YAW_RIGHT = 10;
inline constexpr int KEY_ROLL_LEFT = 11;
inline constexpr int KEY_ROLL_RIGHT = 12;
inline constexpr int KEY_CENTER = 13;
inline constexpr int KEY_ROLL_ENABLE = 14;

inline constexpr int KEY_ACTION_0 = 15;
inline constexpr int KEY_ACTION_1 = 16;
inline constexpr int KEY_ACTION_2 = 17;
inline constexpr int KEY_ACTION_3 = 18;

inline constexpr int KEY_CONTROL_MODEL = 19;
inline constexpr int KEY_MOUSE_SELECT = 20;
inline constexpr int KEY_MOUSE_SENSE = 21;
inline constexpr int KEY_MOUSE_SWAP = 22;
inline constexpr int KEY_MOUSE_INVERT = 23;
inline constexpr int KEY_MOUSE_ACTIVE = 24;

inline constexpr int KEY_JOY_SELECT = 25;
inline constexpr int KEY_JOY_THROTTLE = 26;
inline constexpr int KEY_JOY_RUDDER = 27;
inline constexpr int KEY_JOY_SENSE = 28;
inline constexpr int KEY_JOY_DEAD_ZONE = 29;
inline constexpr int KEY_JOY_SWAP = 30;

inline constexpr int KEY_AXIS_YAW = 32;
inline constexpr int KEY_AXIS_PITCH = 33;
inline constexpr int KEY_AXIS_ROLL = 34;
inline constexpr int KEY_AXIS_THROTTLE = 35;

inline constexpr int KEY_AXIS_YAW_INVERT = 38;
inline constexpr int KEY_AXIS_PITCH_INVERT = 39;
inline constexpr int KEY_AXIS_ROLL_INVERT = 40;
inline constexpr int KEY_AXIS_THROTTLE_INVERT = 41;

// CONTROL VALUES:
//
// joystick buttons and switches must use
// ids greater than 255 so they don't interfere
// with extended ascii numbers for keyboard keys

inline constexpr int KEY_JOY_AXIS_X = 0x1A0;
inline constexpr int KEY_JOY_AXIS_Y = 0x1A1;
inline constexpr int KEY_JOY_AXIS_Z = 0x1A2;
inline constexpr int KEY_JOY_AXIS_RX = 0x1A3;
inline constexpr int KEY_JOY_AXIS_RY = 0x1A4;
inline constexpr int KEY_JOY_AXIS_RZ = 0x1A5;
inline constexpr int KEY_JOY_AXIS_S0 = 0x1A6;
inline constexpr int KEY_JOY_AXIS_S1 = 0x1A7;

inline constexpr int KEY_JOY_1 = 0x1C1;
inline constexpr int KEY_JOY_2 = 0x1C2;
inline constexpr int KEY_JOY_3 = 0x1C3;
inline constexpr int KEY_JOY_4 = 0x1C4;
inline constexpr int KEY_JOY_5 = 0x1C5;
inline constexpr int KEY_JOY_6 = 0x1C6;
inline constexpr int KEY_JOY_7 = 0x1C7;
inline constexpr int KEY_JOY_8 = 0x1C8;
inline constexpr int KEY_JOY_9 = 0x1C9;
inline constexpr int KEY_JOY_10 = 0x1CA;
inline constexpr int KEY_JOY_11 = 0x1CB;
inline constexpr int KEY_JOY_12 = 0x1CC;
inline constexpr int KEY_JOY_13 = 0x1CD;
inline constexpr int KEY_JOY_14 = 0x1CE;
inline constexpr int KEY_JOY_15 = 0x1CF;
inline constexpr int KEY_JOY_16 = 0x1D0;

inline constexpr int KEY_JOY_32 = 0x1E0;

inline constexpr int KEY_POV_0_UP = 0x1F0;
inline constexpr int KEY_POV_0_DOWN = 0x1F1;
inline constexpr int KEY_POV_0_LEFT = 0x1F2;
inline constexpr int KEY_POV_0_RIGHT = 0x1F3;

inline constexpr int KEY_POV_1_UP = 0x1F4;
inline constexpr int KEY_POV_1_DOWN = 0x1F5;
inline constexpr int KEY_POV_1_LEFT = 0x1F6;
inline constexpr int KEY_POV_1_RIGHT = 0x1F7;

inline constexpr int KEY_POV_2_UP = 0x1F8;
inline constexpr int KEY_POV_2_DOWN = 0x1F9;
inline constexpr int KEY_POV_2_LEFT = 0x1FA;
inline constexpr int KEY_POV_2_RIGHT = 0x1FB;

inline constexpr int KEY_POV_3_UP = 0x1FC;
inline constexpr int KEY_POV_3_DOWN = 0x1FD;
inline constexpr int KEY_POV_3_LEFT = 0x1FE;
inline constexpr int KEY_POV_3_RIGHT = 0x1FF;

// +--------------------------------------------------------------------+

class MotionController
{
public:
	static const char* TYPENAME() { return "MotionController"; }

	MotionController()
		: status(StatusOK), sensitivity(1), dead_zone(0),
		swapped(0), inverted(0), rudder(0), throttle(0), select(0)
	{
	}

	virtual ~MotionController() {}

	enum StatusValue { StatusOK, StatusErr, StatusBadParm };
	enum ActionValue { MaxActions = 32 };

	StatusValue    Status()          const { return status; }
	int            Sensitivity()     const { return sensitivity; }
	int            DeadZone()        const { return dead_zone; }
	int            Swapped()         const { return swapped; }
	int            Inverted()        const { return inverted; }
	int            RudderEnabled()   const { return rudder; }
	int            ThrottleEnabled() const { return throttle; }
	int            Selector()        const { return select; }

	// setup:
	virtual void   SetSensitivity(int sense, int dead)
	{
		if (sense > 0) sensitivity = sense;
		if (dead > 0) dead_zone = dead;
	}

	virtual void   SetSelector(int sel) { select = sel; }
	virtual void   SetRudderEnabled(int rud) { rudder = rud; }
	virtual void   SetThrottleEnabled(int t) { throttle = t; }

	virtual void   SwapYawRoll(int swap) { swapped = swap; }
	virtual int    GetSwapYawRoll() { return swapped; }
	virtual void   InvertPitch(int inv) { inverted = inv; }
	virtual int    GetInverted() { return inverted; }

	virtual void   MapKeys(KeyMapEntry* mapping, int nkeys) { (void)mapping; (void)nkeys; }

	// sample the physical device:
	virtual void   Acquire() {}

	// translations:
	virtual double X() { return 0; }
	virtual double Y() { return 0; }
	virtual double Z() { return 0; }

	// rotations:
	virtual double Pitch() { return 0; }
	virtual double Roll() { return 0; }
	virtual double Yaw() { return 0; }
	virtual int    Center() { return 0; }

	// throttle:
	virtual double Throttle() { return 0; }
	virtual void   SetThrottle(double t) { (void)t; }

	// actions:
	virtual int    Action(int n) { (void)n; return 0; }
	virtual int    ActionMap(int n) { (void)n; return 0; }

protected:
	StatusValue    status;
	int            sensitivity;
	int            dead_zone;
	int            swapped;
	int            inverted;
	int            rudder;
	int            throttle;
	int            select;
};
