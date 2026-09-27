/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         HUDSounds.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	HUDSounds singleton class utility
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

class HUDSounds
{
public:
	enum SOUNDS {
		SND_MFD_MODE,
		SND_NAV_MODE,
		SND_WEP_MODE,
		SND_WEP_DISP,
		SND_HUD_MODE,
		SND_HUD_WIDGET,
		SND_SHIELD_LEVEL,
		SND_RED_ALERT,
		SND_TAC_ACCEPT,
		SND_TAC_REJECT
	};

	static void Initialize();
	static void Close();

	static void PlaySound(int n);
	static void StopSound(int n);
};
