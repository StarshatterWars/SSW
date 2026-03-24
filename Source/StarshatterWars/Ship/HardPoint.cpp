/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         HardPoint.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	HardPoint class
*/

#include "HardPoint.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include <cstring>

#include "Weapon.h"
#include "WeaponDesign.h"
#include "SimShot.h"
#include "Ship.h"
#include "Sim.h"

// +----------------------------------------------------------------------+

HardPoint::HardPoint(FVector muzzle_loc, double az, double el)
	: aim_azimuth((float)az)
	, aim_elevation((float)el)
	, muzzle(muzzle_loc)
{
	std::memset(designs, 0, sizeof(designs));
}

// +----------------------------------------------------------------------+

HardPoint::HardPoint(const HardPoint& h)
	: aim_azimuth(h.aim_azimuth)
	, aim_elevation(h.aim_elevation)
	, muzzle(h.muzzle)
	, mount_rel(h.mount_rel)
	, radius(h.radius)
	, hull_factor(h.hull_factor)
{
	std::memcpy(designs, h.designs, sizeof(designs));
}

// +--------------------------------------------------------------------+

HardPoint::~HardPoint()
{
}

// +--------------------------------------------------------------------+

void
HardPoint::Mount(FVector loc, float rad, float hull)
{
	mount_rel = loc;
	radius = rad;
	hull_factor = hull;
}

// +--------------------------------------------------------------------+

void
HardPoint::AddDesign(WeaponDesign* d)
{
	for (int i = 0; i < MAX_DESIGNS; i++) {
		if (!designs[i]) {
			designs[i] = d;
			return;
		}
	}
}

// +--------------------------------------------------------------------+

Weapon*
HardPoint::CreateWeapon(int type_index)
{
	if (type_index >= 0 && type_index < MAX_DESIGNS && designs[type_index]) {
		FVector  zero_pt = FVector::ZeroVector;
		FVector* muzzle_pt = &zero_pt;

		if (designs[type_index]->turret.length() == 0)
			muzzle_pt = &muzzle;

		Weapon* missile = new Weapon(designs[type_index],
			1,
			muzzle_pt,
			aim_azimuth,
			aim_elevation);
		missile->SetAbbreviation(GetAbbreviation());
		missile->Mount(mount_rel, radius, hull_factor);
		return missile;
	}

	return 0;
}

// +--------------------------------------------------------------------+

double
HardPoint::GetCarryMass(int type_index)
{
	if (type_index >= 0 && type_index < MAX_DESIGNS && designs[type_index])
		return designs[type_index]->carry_mass;

	return 0;
}
