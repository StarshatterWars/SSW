/*  Starshatter Wars
	Fractal Dev Studios
	Copyright(C) 2025 - 2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO :
	John DiCamillo, Destroyer Studios LLC

	SUBSYSTEM : nGenEx.lib
	FILE : EventDispatch.h
	AUTHOR : Carlos Bott


	OVERVIEW
	========
	Event Dispatch class
*/

#pragma once

#include "Types.h"
#include "EventTarget.h"
#include "List.h"

// +--------------------------------------------------------------------+

class EventDispatch
{
public:
	static const char* TYPENAME() { return "EventDispatch"; }

	EventDispatch();
	virtual ~EventDispatch();

	static void           Create();
	static void           Close();
	static EventDispatch* GetInstance() { return dispatcher; }

	virtual void          Dispatch();
	virtual void          Register(EventTarget* tgt);
	virtual void          Unregister(EventTarget* tgt);

	virtual EventTarget*  GetCapture();
	virtual int           CaptureMouse(EventTarget* tgt);
	virtual int           ReleaseMouse(EventTarget* tgt);

	virtual EventTarget* GetFocus();
	virtual void          SetFocus(EventTarget* tgt);
	virtual void          KillFocus(EventTarget* tgt);

	virtual void          MouseEnter(EventTarget* tgt);

protected:
	int               mouse_x, mouse_y, mouse_l, mouse_r;
	List<EventTarget> clients;
	EventTarget* capture;
	EventTarget* current;
	EventTarget* focus;
	EventTarget* click_tgt;

	static EventDispatch* dispatcher;
};
