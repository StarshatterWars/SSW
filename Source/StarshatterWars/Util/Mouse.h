/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Mouse.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Mouse class
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

class Bitmap;
class Screen;
class View;

#if PLATFORM_WINDOWS
struct HWND__;
using HWND = HWND__*;

using UINT = unsigned int;

// Prefer the Unreal/Windows-agnostic pointer-sized types:
using WPARAM = UPTRINT;
using LPARAM = PTRINT;
using LRESULT = PTRINT;
#endif
// +--------------------------------------------------------------------+

class Mouse
{
#if PLATFORM_WINDOWS
	friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
#endif

	friend class Game;

public:
	static const char* TYPENAME() { return "Mouse"; }

	enum CURSOR { ARROW, CROSS, WAIT, NOT, DRAG, USER1, USER2, USER3 };
	enum HOTSPOT { HOTSPOT_CTR, HOTSPOT_NW };

	static int     X() { return x; }
	static int     Y() { return y; }
	static int     LButton() { return l; }
	static int     MButton() { return m; }
	static int     RButton() { return r; }
	static int     Wheel() { return w; }

	static void    Paint();

	static void    SetCursorPos(int x, int y);
	static void    Show(int s = 1);
	static int     SetCursor(CURSOR c);
	static int     LoadCursor(CURSOR c, const char* name, HOTSPOT hs = HOTSPOT_CTR);

	static void    Create(Screen* screen);
	static void    Resize(Screen* screen);
	static void    Close();

private:
	static int     show;
	static int     cursor;

	static int     x;
	static int     y;
	static int     l;
	static int     m;
	static int     r;
	static int     w;

	static Bitmap* image[8];
	static int     hotspot[8];

	static View* window;
};
