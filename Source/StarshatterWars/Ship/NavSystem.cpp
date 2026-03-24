/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	Original Author and Studio: John DiCamillo, Destroyer Studios LLC
	SUBSYSTEM:    Stars.exe
	FILE:         NavSystem.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Navigation System class implementation
*/

#include "NavSystem.h"

#include "Ship.h"
#include "Sim.h"
#include "HUDSounds.h"
#include "UIButton.h"
#include "Game.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogNavSystem, Log, All);

// +----------------------------------------------------------------------+

NavSystem::NavSystem()
	: SimSystem(SYSTEM_CATEGORY::COMPUTER, 2, "Auto Nav System", 1, 1, 1, 1),
	autonav(false)
{
	name = "Auto Nav System";
	abrv = "Nav";

	power_flags = POWER_WATTS | POWER_CRITICAL;
}

// +----------------------------------------------------------------------+

NavSystem::NavSystem(const NavSystem& s)
	: SimSystem(s),
	autonav(false)
{
	Mount(s);

	power_flags = POWER_WATTS | POWER_CRITICAL;
}

// +--------------------------------------------------------------------+

NavSystem::~NavSystem()
{
}

// +--------------------------------------------------------------------+

void NavSystem::ExecFrame(double seconds)
{
	if (autonav && ship && !ship->GetNextNavPoint())
		autonav = false;

	energy = 0.0f;
	SimSystem::ExecFrame(seconds);
}

// +----------------------------------------------------------------------+

bool NavSystem::AutoNavEngaged()
{
	return ship && autonav && IsPowerOn();
}

void NavSystem::EngageAutoNav()
{
	if (!IsPowerOn() || autonav)
		return;

	if (!ship) {
		UE_LOG(LogNavSystem, Warning, TEXT("EngageAutoNav called with null ship."));
		return;
	}

	if (!ship->GetNextNavPoint()) {
		UIButton::PlaySound(UIButton::SND_REJECT);
	}
	else {
		HUDSounds::PlaySound(HUDSounds::SND_NAV_MODE);
		autonav = true;
	}
}

void NavSystem::DisengageAutoNav()
{
	if (autonav)
		HUDSounds::PlaySound(HUDSounds::SND_NAV_MODE);

	autonav = false;
}

// +--------------------------------------------------------------------+

void NavSystem::Distribute(double delivered_energy, double seconds)
{
	if (!IsPowerOn())
		return;

	// Defensive: avoid divide-by-zero / negative step.
	if (seconds <= 0) {
		UE_LOG(LogNavSystem, Warning, TEXT("Distribute called with non-positive seconds (%.6f)."), seconds);
		return;
	}

	// convert Joules to Watts:
	energy = (float)(delivered_energy / seconds);

	// brown out:
	if (energy < capacity * 0.75f) {
		power_on = false;
	}

	// spike:
	else if (energy > capacity * 1.5f) {
		power_on = false;
		ApplyDamage(50);
	}
}
