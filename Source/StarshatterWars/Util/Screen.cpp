/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Screen.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	General Screen class - maintains and displays a list of windows
*/

#include "Screen.h"

#include "Bitmap.h"
#include "Color.h"
#include "Mouse.h"
#include "Video.h"
#include "View.h"

#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

Screen::Screen(Video* v)
	: width(0), height(0), clear(0), closed(0), video(v)
{
	if (video) {
		width = video->Width();
		height = video->Height();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Screen::Screen: video is null"));
	}

	Mouse::Create(this);
}

Screen::~Screen()
{
	Mouse::Close();

	closed = 1;
	window_list.destroy();
}

// +--------------------------------------------------------------------+

bool
Screen::AddWindow(class View* c)
{
	if (!c || closed) return false;

	if (c->X() < 0) return false;
	if (c->Y() < 0) return false;
	if (c->X() + c->Width() > Width())  return false;
	if (c->Y() + c->Height() > Height()) return false;

	if (!window_list.contains(c))
		window_list.append(c);

	return true;
}

bool Screen::DelWindow(View* c)
{
	if (!c || closed)
		return false;

	if (window_list.contains(c))
		window_list.remove(c);     

	return true;
}
// +--------------------------------------------------------------------+

void
Screen::ClearAllFrames(bool clear_all)
{
	if (clear_all)
		clear = -1;
	else
		clear = 0;
}

void
Screen::ClearNextFrames(int num_frames)
{
	if (clear >= 0 && clear < num_frames)
		clear = num_frames;
}

// +--------------------------------------------------------------------+

bool
Screen::SetBackgroundColor(FColor c)
{
	if (video)
		return video->SetBackgroundColor(c);

	return false;
}

// +--------------------------------------------------------------------+

bool
Screen::Resize(int w, int h)
{
	if (w <= 0 || h <= 0) {
		UE_LOG(LogTemp, Warning, TEXT("Screen::Resize: invalid size %d x %d"), w, h);
		return false;
	}

	if (width <= 0 || height <= 0) {
		width = w;
		height = h;
		return true;
	}

	// scale all root-level windows to new screen size:
	ListIter<View> iter = window_list;
	while (++iter) {
		View* win = iter.value();
		if (!win) continue;

		Rect tmprect = win->GetRect();

		const double w_x = tmprect.x / (double)width;
		const double w_y = tmprect.y / (double)height;
		const double w_w = tmprect.w / (double)width;
		const double w_h = tmprect.h / (double)height;

		Rect r;
		r.x = (int)(w_x * w);
		r.y = (int)(w_y * h);
		r.w = (int)(w_w * w);
		r.h = (int)(w_h * h);

		win->MoveTo(r);
	}

	width = w;
	height = h;

	return true;
}

// +--------------------------------------------------------------------+

bool
Screen::Refresh()
{
	if (!video) {
		UE_LOG(LogTemp, Warning, TEXT("Screen::Refresh: video is null"));
		return false;
	}

	if (clear && !video->ClearAll())
		return false;

	video->StartFrame();

	ListIter<View> iter = window_list;
	while (++iter) {
		View* win = iter.value();

		if (win && win->IsShown()) {
			win->Paint();
		}
	}

	Mouse::Paint();

	video->EndFrame();

	if (clear > 0) clear--;
	return true;
}
