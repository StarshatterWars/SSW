/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Keyboard.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Keyboard Input class
*/

#include "Keyboard.h"
#include "Game.h"

#include "HAL/Platform.h"
#include "Logging/LogMacros.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogKeyboard, Log, All);

// +--------------------------------------------------------------------+

static Keyboard* instance = 0;
int    Keyboard::map[KEY_MAP_SIZE];
int    Keyboard::alt[KEY_MAP_SIZE];

Keyboard::Keyboard()
	: x(0), y(0), z(0), p(0), r(0), w(0), c(0), p1(0), r1(0), w1(0), t(0)
{
	instance = this;
	sensitivity = 25;
	dead_zone = 100;

	for (int i = 0; i < MotionController::MaxActions; i++)
		action[i] = 0;

	memset(map, 0, sizeof(map));
	memset(alt, 0, sizeof(alt));

	map[KEY_PLUS_X] = 'R';
	map[KEY_MINUS_X] = 'E';

#if PLATFORM_WINDOWS
	map[KEY_PLUS_Y] = VK_HOME;
	map[KEY_MINUS_Y] = VK_END;
	map[KEY_PLUS_Z] = VK_PRIOR;    // page up
	map[KEY_MINUS_Z] = VK_NEXT;     // page down

	map[KEY_PITCH_UP] = VK_DOWN;
	map[KEY_PITCH_DOWN] = VK_UP;
	map[KEY_YAW_LEFT] = VK_LEFT;
	map[KEY_YAW_RIGHT] = VK_RIGHT;
	map[KEY_ROLL_ENABLE] = 0;           // used to be VK_CONTROL;
#else
	// Non-Windows: fall back to 0 (unbound) until a platform-specific key system is added:
	map[KEY_PLUS_Y] = 0;
	map[KEY_MINUS_Y] = 0;
	map[KEY_PLUS_Z] = 0;
	map[KEY_MINUS_Z] = 0;

	map[KEY_PITCH_UP] = 0;
	map[KEY_PITCH_DOWN] = 0;
	map[KEY_YAW_LEFT] = 0;
	map[KEY_YAW_RIGHT] = 0;
	map[KEY_ROLL_ENABLE] = 0;
#endif
}

Keyboard::~Keyboard()
{
	instance = 0;
}

Keyboard*
Keyboard::GetInstance()
{
	return instance;
}

// +--------------------------------------------------------------------+

void
Keyboard::MapKeys(KeyMapEntry* mapping, int nkeys)
{
	if (!mapping || nkeys <= 0)
		return;

	for (int i = 0; i < nkeys; i++) {
		const KeyMapEntry& Entry = mapping[i];

		if (Entry.act >= KEY_MAP_FIRST && Entry.act <= KEY_MAP_LAST) {
#if PLATFORM_WINDOWS
			if (Entry.key == 0 || (Entry.key > VK_MBUTTON && Entry.key < KEY_JOY_1)) {
				map[Entry.act] = Entry.key;
				alt[Entry.act] = Entry.alt;
			}
#else
			// Legacy non-Windows fallback
			map[Entry.act] = Entry.key;
			alt[Entry.act] = Entry.alt;
#endif
		}
	}
}


// +--------------------------------------------------------------------+

bool Keyboard::KeyDown(int key)
{
#if PLATFORM_WINDOWS
	if (key) {
		const short KeyState = GetAsyncKeyState(key);
		return (KeyState < 0) || (KeyState & 1);
	}
	return false;
#else
	(void)key;
	return false;
#endif
}

// +--------------------------------------------------------------------+

bool Keyboard::KeyDownMap(int key)
{
#if PLATFORM_WINDOWS
	if (key >= KEY_MAP_FIRST && key <= KEY_MAP_LAST && map[key]) {
		const short PrimaryState = GetAsyncKeyState(map[key]);
		short AltState = -1;

		if (alt[key] > 0 && alt[key] < KEY_JOY_1) {
			AltState = GetAsyncKeyState(alt[key]);
		}
		else {
			AltState = !GetAsyncKeyState(VK_SHIFT) &&
				!GetAsyncKeyState(VK_MENU);
		}

		return ((PrimaryState < 0) || (PrimaryState & 1)) &&
			((AltState < 0) || (AltState & 1));
	}

	return false;
#else
	(void)key;
	return false;
#endif
}

// +--------------------------------------------------------------------+

void
Keyboard::FlushKeys()
{
#if PLATFORM_WINDOWS
	for (int i = 0; i < 255; i++)
		GetAsyncKeyState(i);
#endif
}

// +--------------------------------------------------------------------+

static inline double sqr(double a) { return a; } //*a; }

void
Keyboard::Acquire()
{
	t = x = y = z = p = r = w = c = 0;

	for (int i = 0; i < MotionController::MaxActions; i++)
		action[i] = 0;

	// lateral translations:
	if (KeyDownMap(KEY_PLUS_Y))            y = 1;
	else if (KeyDownMap(KEY_MINUS_Y))      y = -1;

	if (KeyDownMap(KEY_PLUS_Z))            z = 1;
	else if (KeyDownMap(KEY_MINUS_Z))      z = -1;

	if (KeyDownMap(KEY_MINUS_X))           x = -1;
	else if (KeyDownMap(KEY_PLUS_X))       x = 1;

	const double steps = 10;

	// if roll and yaw are swapped --------------------------
	if (swapped) {
		// yaw:
		if (KeyDownMap(KEY_ROLL_LEFT)) { if (w1 < steps) w1 += 1; w = -sqr(w1 / steps); }
		else if (KeyDownMap(KEY_ROLL_RIGHT)) { if (w1 < steps) w1 += 1; w = sqr(w1 / steps); }

		// another way to yaw:
		if (KeyDownMap(KEY_ROLL_ENABLE)) {
			if (KeyDownMap(KEY_YAW_LEFT)) { if (w1 < steps) w1 += 1; w = -sqr(w1 / steps); }
			else if (KeyDownMap(KEY_YAW_RIGHT)) { if (w1 < steps) w1 += 1; w = sqr(w1 / steps); }
			else w1 = 0;
		}

		// roll:
		else {
			if (KeyDownMap(KEY_YAW_LEFT)) { if (r1 < steps) r1 += 1; r = sqr(r1 / steps); }
			else if (KeyDownMap(KEY_YAW_RIGHT)) { if (r1 < steps) r1 += 1; r = -sqr(r1 / steps); }
			else r1 = 0;
		}
	}

	// else roll and yaw are NOT swapped ---------------------
	else {
		// roll:
		if (KeyDownMap(KEY_ROLL_LEFT)) { if (r1 < steps) r1 += 1; r = sqr(r1 / steps); }
		else if (KeyDownMap(KEY_ROLL_RIGHT)) { if (r1 < steps) r1 += 1; r = -sqr(r1 / steps); }

		// another way to roll:
		if (KeyDownMap(KEY_ROLL_ENABLE)) {
			if (KeyDownMap(KEY_YAW_LEFT)) { if (r1 < steps) r1 += 1; r = sqr(r1 / steps); }
			else if (KeyDownMap(KEY_YAW_RIGHT)) { if (r1 < steps) r1 += 1; r = -sqr(r1 / steps); }
			else r1 = 0;
		}

		// yaw left-right
		else {
			if (KeyDownMap(KEY_YAW_LEFT)) { if (w1 < steps) w1 += 1; w = -sqr(w1 / steps); }
			else if (KeyDownMap(KEY_YAW_RIGHT)) { if (w1 < steps) w1 += 1; w = sqr(w1 / steps); }
			else w1 = 0;
		}
	}

	// if pitch is inverted ----------------------------------
	if (inverted) {
		if (KeyDownMap(KEY_PITCH_DOWN)) { if (p1 < steps) p1 += 1; p = -sqr(p1 / steps); }
		else if (KeyDownMap(KEY_PITCH_UP)) { if (p1 < steps) p1 += 1; p = sqr(p1 / steps); }
		else p1 = 0;
	}

	// else pitch is NOT inverted ----------------------------
	else {
		if (KeyDownMap(KEY_PITCH_UP)) { if (p1 < steps) p1 += 1; p = -sqr(p1 / steps); }
		else if (KeyDownMap(KEY_PITCH_DOWN)) { if (p1 < steps) p1 += 1; p = sqr(p1 / steps); }
		else p1 = 0;
	}

	if (KeyDownMap(KEY_CENTER))            c = 1;

	// actions
	if (KeyDownMap(KEY_ACTION_0))          action[0] = 1;
	if (KeyDownMap(KEY_ACTION_1))          action[1] = 1;
	if (KeyDownMap(KEY_ACTION_2))          action[2] = 1;
	if (KeyDownMap(KEY_ACTION_3))          action[3] = 1;
}

// +--------------------------------------------------------------------+
