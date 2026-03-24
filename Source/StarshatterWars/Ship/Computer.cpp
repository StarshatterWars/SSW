/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Computer.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Computer System class
*/

#include "Computer.h"

#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "Game.h"

// +----------------------------------------------------------------------+

static int computer_value[] = {
	0, 1, 1, 1, 1
};

// +----------------------------------------------------------------------+

Computer::Computer(int comp_type, const char* comp_name)
	: SimSystem(SYSTEM_CATEGORY::COMPUTER, comp_type, comp_name, 1, 1, 1, 1)
{
	SetAbbreviation("COMP");
	power_flags = POWER_WATTS | POWER_CRITICAL;

	if (subtype == FLIGHT) {
		crit_level = -1.0f;
	}
}

// +----------------------------------------------------------------------+

Computer::Computer(const Computer& c)
	: SimSystem(c)
{
	Mount(c);
	SetAbbreviation(c.Abbreviation());
	power_flags = POWER_WATTS | POWER_CRITICAL;

	if (subtype == FLIGHT) {
		crit_level = -1.0f;
	}
}

// +--------------------------------------------------------------------+

Computer::~Computer()
{
}

// +--------------------------------------------------------------------+

void
Computer::ApplyDamage(double damage)
{
	SimSystem::ApplyDamage(damage);
}

// +--------------------------------------------------------------------+

void
Computer::ExecFrame(double seconds)
{
	energy = 0.0f;
	SimSystem::ExecFrame(seconds);
}

// +--------------------------------------------------------------------+

void
Computer::Distribute(double delivered_energy, double seconds)
{
	if (IsPowerOn()) {
		// convert Joules to Watts:
		energy = (float)(delivered_energy / seconds);

		// brown out:
		if (energy < capacity * 0.75f)
			power_on = false;

		// spike:
		else if (energy > capacity * 1.5f) {
			power_on = false;
			ApplyDamage(50);
		}
	}
}
