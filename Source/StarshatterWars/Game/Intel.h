// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Types.h"
#include "GameStructs.h"

/**
 * 
 */

class STARSHATTERWARS_API Intel
{

public:
	static EIntel      GetIntelFromName(const char* name);
	static const char* GetNameFromIntel(EIntel intel);
};
