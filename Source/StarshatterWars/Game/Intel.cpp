// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "Intel.h"
#include "Game.h"
#include "DataLoader.h"

// +--------------------------------------------------------------------+

static const char* intel_name[] = {
	"",
	"Reserve",
	"Secret",
	"Known",
	"Located",
	"Tracked",
};

const char*
Intel::NameFromIntel(int intel)
{
	return intel_name[intel];
}

int
Intel::IntelFromName(const char* type_name)
{
	for (int i = RESERVE; i <= TRACKED; i++)
		if (!_stricmp(type_name, intel_name[i]))
			return i;

	return 0;
}
