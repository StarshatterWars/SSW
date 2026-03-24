// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Types.h"

/**
 * 
 */

class STARSHATTERWARS_API Intel
{

public:
	enum INTEL_TYPE {
		RESERVE = 1,   // out-system reserve: this group is notxeven here
		SECRET,        // enemy is completely unaware of this group
		KNOWN,         // enemy knows this group is in the system
		LOCATED,       // enemy has located at least the lead ship
		TRACKED        // enemy is tracking all elements
	};

	static int         IntelFromName(const char* name);
	static const char* NameFromIntel(int intel);
};
