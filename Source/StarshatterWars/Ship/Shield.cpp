/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Shield.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Shield system class
*/

#include "Shield.h"

#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "SimShot.h"
#include "WeaponDesign.h"
#include "Game.h"

// +----------------------------------------------------------------------+

static const char* shield_name[] = {
	"sys.shield.none",
	"sys.shield.deflector",
	"sys.shield.grav",
	"sys.shield.hyper"
};

static int shield_value[] = {
	0, 2, 2, 3
};

// +----------------------------------------------------------------------+

Shield::Shield(SUBTYPE shield_type)
	: SimSystem(SYSTEM_CATEGORY::SHIELD, shield_type, "shield", shield_value[shield_type], 100, 0)
	, shield_capacitor(false)
	, shield_bubble(false)
	, shield_factor(0.0f)
	, shield_level(0.0f)
	, shield_curve(0.05f)
	, shield_cutoff(0.0f)
	, requested_power_level(0.0f)
	, deflection_cost(1.0f)
{
	name = Game::GetText(shield_name[shield_type]);
	abrv = Game::GetText("sys.shield.abrv");

	power_flags = POWER_WATTS | POWER_CRITICAL;
	energy = 0.0f;
	power_level = 0.0f;
	shield_level = 0.0f;

	switch (shield_type) {
	default:
	case DEFLECTOR:
		capacity = 2.0e3f;
		sink_rate = 2.0e3f;
		shield_factor = 0.05f;
		break;

	case GRAV_SHIELD:
		capacity = 7.0e3f;
		sink_rate = 7.0e3f;
		shield_factor = 0.01f;
		break;

	case HYPER_SHIELD:
		capacity = 10.0e3f;
		sink_rate = 10.0e3f;
		shield_factor = 0.003f;
		break;
	}

	emcon_power[0] = 0;
	emcon_power[1] = 0;
	emcon_power[2] = 100;
}

// +----------------------------------------------------------------------+

Shield::Shield(const Shield& s)
	: SimSystem(s)
	, shield_capacitor(s.shield_capacitor)
	, shield_bubble(s.shield_bubble)
	, shield_factor(s.shield_factor)
	, shield_level(0.0f)
	, shield_curve(s.shield_curve)
	, shield_cutoff(s.shield_cutoff)
	, requested_power_level(0.0f)
	, deflection_cost(s.deflection_cost)
{
	power_flags = s.power_flags;
	energy = 0.0f;
	power_level = 0.0f;
	shield_level = 0.0f;

	Mount(s);
}

// +--------------------------------------------------------------------+

Shield::~Shield()
{
}

void
Shield::SetShieldCapacitor(bool c)
{
	shield_capacitor = c;

	if (shield_capacitor) {
		power_flags = POWER_CRITICAL;
		shield_curve = 0.05f;
	}
	else {
		power_flags = POWER_WATTS | POWER_CRITICAL;
		shield_curve = 0.25f;
	}
}

// +--------------------------------------------------------------------+

void
Shield::ExecFrame(double seconds)
{
	SimSystem::ExecFrame(seconds);

	if (power_level < requested_power_level) {
		power_level += (float)(seconds * 0.10);     // ten seconds to charge up

		if (power_level > requested_power_level)
			power_level = (float)requested_power_level;
	}
	else if (power_level > requested_power_level) {
		power_level -= (float)(seconds * 0.20);     // five seconds to power down

		if (power_level < requested_power_level)
			power_level = (float)requested_power_level;
	}

	if (power_level < 0.01f && !shield_capacitor) {
		shield_level = 0.0f;
		energy = 0.0f;
	}
}

// +----------------------------------------------------------------------+

void
Shield::Distribute(double delivered_energy, double seconds)
{
	SimSystem::Distribute(delivered_energy, seconds);

	if (shield_capacitor) {
		if (shield_cutoff > 0 && shield_cutoff < 0.999f) {
			float cutoff = shield_cutoff * capacity;

			if (energy > cutoff)
				shield_level = (energy - cutoff) / (capacity - cutoff);
			else
				shield_level = 0.0f;
		}
		else {
			shield_level = energy / capacity;
		}
	}
	else {
		shield_level = energy / sink_rate;
		energy = 0.0f;
	}

	if (shield_level < 0)
		shield_level = 0;
}

// +--------------------------------------------------------------------+

double
Shield::DeflectDamage(SimShot* shot, double damage)
{
	double filter = 1.0;
	double penetration = 5.0;
	double leak = 0.0;

	if (shot)
		penetration = shot->Design()->penetration;

	filter = 1.0 - shield_factor * penetration;

	if (filter < 0.0)      filter = 0.0;
	else if (filter > 1.0) filter = 1.0;

	if (shield_capacitor) {
		if (shield_cutoff > 0 && shield_level < 1e-6f) {
			leak = damage;
			energy -= (float)(damage * deflection_cost);
		}
		else {
			leak = damage * (1.0 - FMath::Pow((double)shield_level, (double)shield_curve) * filter * availability);

			const double deflected = damage - leak;
			energy -= (float)(deflected * deflection_cost);
		}
	}
	else {
		leak = damage * (1.0 - FMath::Pow((double)shield_level, (double)shield_curve) * filter * availability);
	}

	return leak;
}

// +--------------------------------------------------------------------+

void
Shield::SetPowerLevel(double level)
{
	if (level > 100)      level = 100;
	else if (level < 0)   level = 0;

	level /= 100.0;

	if (requested_power_level != level) {
		// if the system is on emergency override power,
		// do not let the EMCON system use this method
		// to drop it back to normal power:
		if (power_level > 1.0f && level == 1.0) {
			requested_power_level = (float)power_level;
			return;
		}

		requested_power_level = (float)level;
	}
}

void
Shield::SetNetShieldLevel(int level)
{
	if (level > 100)      level = 100;
	else if (level < 0)   level = 0;

	requested_power_level = (float)(level / 100.0);
	power_level = requested_power_level;
}

void
Shield::DoEMCON(int index)
{
	int e = GetEMCONPower(index);

	if (power_level * 100 > e || emcon != index) {
		if (e == 0) {
			PowerOff();
		}
		else if (emcon != index) {
			PowerOn();

			if (power_level * 100 > e)
				SetPowerLevel(e);
		}
	}

	emcon = index;
}
