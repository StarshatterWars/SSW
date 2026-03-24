/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         SimUniverse.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Abstract Universe class
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

class SimUniverse
{
public:
	SimUniverse() {}
	virtual ~SimUniverse() {}

	virtual void ExecFrame(double seconds) {}
};
