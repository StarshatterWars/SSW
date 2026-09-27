/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	Original Author and Studio: John DiCamillo, Destroyer Studios LLC
	SUBSYSTEM:    Stars.exe
	FILE:         NavSystem.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Nav Points and so on...
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "SimSystem.h"

// +--------------------------------------------------------------------+

class StarSystem;
class Orbital;
class OrbitalBody;
class OrbitalRegion;
class Ship;

// +--------------------------------------------------------------------+

class NavSystem : public SimSystem
{
public:
	NavSystem();
	NavSystem(const NavSystem& rhs);
	virtual ~NavSystem();

	virtual void   ExecFrame(double seconds) override;

	virtual void   Distribute(double delivered_energy, double seconds) override;

	bool           AutoNavEngaged();
	void           EngageAutoNav();
	void           DisengageAutoNav();

protected:
	bool           autonav = false;
};

// +--------------------------------------------------------------------+
