/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioVox.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	OVERVIEW
	========
	View class for Radio Communications HUD Overlay
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"

// +--------------------------------------------------------------------+

class Element;
class Ship;
class RadioMessage;
class USound;

// +--------------------------------------------------------------------+

class RadioVox
{
	friend class RadioVoxController;

public:
	static const char* TYPENAME() { return "RadioVox"; }

	RadioVox(int channel, const char* path, const char* message = 0);
	virtual ~RadioVox();

	// Operations:
	virtual bool      AddPhrase(const char* key);
	virtual bool      Start();

	static void       Initialize();
	static void       Close();

protected:
	virtual bool      Update();

	Text              path;
	Text              message;
	List<USound>      sounds;
	int               index;
	int               channel;
};
