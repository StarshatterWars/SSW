/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         Types.h
	AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <assert.h>
#include <math.h>      
#include <limits.h>      
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Windows/MinWindows.h"

// ---------------------------------------------------------------------
// Minimal VK_* keys used by legacy code (MinWindows may not provide these)
// ---------------------------------------------------------------------
#ifndef VK_RETURN
#define VK_RETURN 0x0D
#endif

#ifndef VK_ESCAPE
#define VK_ESCAPE 0x1B
#endif

// ---------------------------------------------------------------------
// Legacy DrawText-style flags (DT_*)
// WinUser.h defines these, but we are NOT including WinUser.h.
// So define only what your UI code uses.
// ---------------------------------------------------------------------
#ifndef DT_LEFT
#define DT_LEFT        0x00000000
#endif

#ifndef DT_CENTER
#define DT_CENTER      0x00000001
#endif

#ifndef DT_RIGHT
#define DT_RIGHT       0x00000002
#endif

#ifndef DT_WORDBREAK
#define DT_WORDBREAK   0x00000010
#endif

#ifndef DT_SINGLELINE
#define DT_SINGLELINE  0x00000020
#endif

// ---------------------------------------------------------------------
// Useful legacy constants
// ---------------------------------------------------------------------
#ifndef PI
#define PI 3.14159265358979323846
#endif