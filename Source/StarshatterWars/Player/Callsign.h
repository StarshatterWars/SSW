/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         Callsign.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Package Callsign catalog class
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

class Callsign
{
public:
	static const char* GetCallsign(int IFF = 1);
};
