/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Mouse.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Mouse class
*/

#include "Mouse.h"

#include "DataLoader.h"
#include "Screen.h"
#include "Bitmap.h"
#include "Video.h"
#include "View.h"

#include "Logging/LogMacros.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h> // POINT, GetCursorPos, SetCursorPos
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogMouse, Log, All);

// +--------------------------------------------------------------------+

int     Mouse::show = 1;
int     Mouse::cursor = Mouse::ARROW;

int     Mouse::x = 320;
int     Mouse::y = 240;
int     Mouse::l = 0;
int     Mouse::m = 0;
int     Mouse::r = 0;
int     Mouse::w = 0;

Bitmap* Mouse::image[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int     Mouse::hotspot[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
View*   Mouse::window = 0;

// +--------------------------------------------------------------------+

void
Mouse::Create(Screen* screen)
{
	if (screen) {
		delete window;
		window = new View(screen, 0, 0, screen->Width(), screen->Height());
	}
}

// +--------------------------------------------------------------------+

void
Mouse::Resize(Screen* screen)
{
	if (screen) {
		delete window;
		window = new View(screen, 0, 0, screen->Width(), screen->Height());
	}
}

// +--------------------------------------------------------------------+

void
Mouse::Close()
{
	for (int i = 0; i < 8; i++) {
		delete image[i];
		image[i] = 0;
	}

	delete window;
	window = 0;

	show = 0;
	cursor = ARROW;
}

// +--------------------------------------------------------------------+

void
Mouse::SetCursorPos(int ax, int ay)
{
#if PLATFORM_WINDOWS
	POINT p;
	int   dx = 0;
	int   dy = 0;

	::GetCursorPos(&p);

	dx = p.x - x;
	dy = p.y - y;

	x = ax;
	y = ay;

	::SetCursorPos(x + dx, y + dy);
#else
	UE_LOG(LogMouse, VeryVerbose, TEXT("Mouse::SetCursorPos is Windows-only; ignoring SetCursorPos(%d,%d)."), ax, ay);
	x = ax;
	y = ay;
#endif
}

// +--------------------------------------------------------------------+

int
Mouse::SetCursor(CURSOR c)
{
	int old = cursor;
	cursor = c;
	return old;
}

// +--------------------------------------------------------------------+

int
Mouse::LoadCursor(CURSOR c, const char* name, HOTSPOT hs)
{
	int result = 0;

	delete image[c];
	image[c] = 0;

	if (name && *name) {
		image[c] = new Bitmap;
		result = DataLoader::GetLoader()->LoadGameBitmap(name, *image[c], Bitmap::BMP_TRANSPARENT);

		if (result) {
			Bitmap* bmp = image[c];

			if (bmp && bmp->HiPixels()) {
#if PLATFORM_WINDOWS
				image[c]->CopyAlphaRedChannel(
					image[c]->Width(),
					image[c]->Height(),
					(LPDWORD)image[c]->HiPixels()
				);
#else
				UE_LOG(LogMouse, VeryVerbose, TEXT("Mouse::LoadCursor: CopyAlphaRedChannel uses Win32 DWORD layout; skipped on this platform."));
#endif
			}

			hotspot[c] = hs;
		}
	}

	return result;
}

// +--------------------------------------------------------------------+

void
Mouse::Show(int s)
{
	show = s;
}

// +--------------------------------------------------------------------+

void
Mouse::Paint()
{
	if (!show || !window)
		return;

	// draw using bitmap:
	if (image[cursor]) {
		int w2 = image[cursor]->Width() / 2;
		int h2 = image[cursor]->Height() / 2;

		if (hotspot[cursor] == HOTSPOT_NW)
			window->DrawBitmap(x, y, x + w2 * 2, y + h2 * 2, image[cursor], Video::BLEND_ALPHA);
		else
			window->DrawBitmap(x - w2, y - h2, x + w2, y + h2, image[cursor], Video::BLEND_ALPHA);
	}

	// draw using primitives (legacy - disabled):
	/***
	else {
		switch (cursor) {
		case ARROW:
		case CROSS:
		case USER1:
		case USER2:
		case USER3:
		default:
			window->DrawLine(x-7, y,   x+8, y,   Color::White);
			window->DrawLine(x,   y-7, x,   y+8, Color::White);
			break;

		case WAIT:
			window->DrawLine(x-7, y-7, x+8, y-7, Color::White);
			window->DrawLine(x-7, y-7, x+8, y+8, Color::White);
			window->DrawLine(x-7, y+8, x+8, y-7, Color::White);
			window->DrawLine(x-7, y+8, x+8, y+8, Color::White);
			break;

		case NOT:
			window->DrawEllipse(x-7,y-7,x+8,y+8, Color::White);
			window->DrawLine(   x-7,y-7,x+8,y+8, Color::White);
			break;

		case DRAG:
			window->DrawRect(x-7, y-6, x+8, y-3, Color::White);
			window->DrawRect(x-7, y-1, x+8, y+2, Color::White);
			window->DrawRect(x-7, y+4, x+8, y+7, Color::White);
			break;
		}
	}
	***/
}
