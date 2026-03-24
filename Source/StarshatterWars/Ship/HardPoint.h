/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         HardPoint.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Hard Point (gun or missile launcher) class
*/

#pragma once

#include "Types.h"
#include "Text.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Weapon;
class WeaponDesign;

// +--------------------------------------------------------------------+

class HardPoint
{
public:
	static const char* TYPENAME() { return "HardPoint"; }

	enum CONSTANTS { MAX_DESIGNS = 8 };

	HardPoint(FVector muzzle, double az = 0, double el = 0);
	HardPoint(const HardPoint& rhs);
	virtual ~HardPoint();

	int operator==(const HardPoint& w) const { return this == &w; }

	virtual void        AddDesign(WeaponDesign* dsn);
	virtual Weapon*     CreateWeapon(int type_index = 0);
	virtual double      GetCarryMass(int type_index = 0);
	WeaponDesign*	    GetWeaponDesign(int n) { return designs[n]; }

	virtual void        Mount(FVector loc, float rad, float hull = 0.5f);
	FVector             MountLocation()               const { return mount_rel; }
	double              Radius()                      const { return radius; }
	double              HullProtection()              const { return hull_factor; }

	virtual const char* GetName()                     const { return name; }
	virtual void        SetName(const char* s) { name = s; }
	virtual const char* GetAbbreviation()             const { return abrv; }
	virtual void        SetAbbreviation(const char* s) { abrv = s; }
	virtual const char* GetDesign()                   const { return sys_dsn; }
	virtual void        SetDesign(const char* s) { sys_dsn = s; }

	virtual double      GetAzimuth()                  const { return aim_azimuth; }
	virtual void        SetAzimuth(double a) { aim_azimuth = (float)a; }
	virtual double      GetElevation()                const { return aim_elevation; }
	virtual void        SetElevation(double e) { aim_elevation = (float)e; }

protected:
	// Displayable name:
	Text               name;
	Text               abrv;
	Text               sys_dsn;

	WeaponDesign* designs[MAX_DESIGNS];
	FVector            muzzle;
	float              aim_azimuth;
	float              aim_elevation;

	// Mounting:
	FVector            mount_rel;  // object space
	float              radius;
	float              hull_factor;
};
