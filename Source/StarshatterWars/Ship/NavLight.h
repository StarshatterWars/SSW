/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         NavLight.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Navigation Lights System class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "DriveSprite.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Physical;
class SimSystem;

class NavLight : public SimSystem
{
public:
	enum Constants { MAX_LIGHTS = 8 };

	NavLight(double period, double scale);
	NavLight(const NavLight& rhs);
	virtual ~NavLight();

	static void    Initialize();
	static void    Close();

	virtual void   ExecFrame(double seconds);

	int            NumBeacons()      const { return nlights; }
	Sprite* Beacon(int index) const { return beacon[index]; }
	bool           IsEnabled()       const { return enable; }

	virtual void   Enable();
	virtual void   Disable();
	virtual void   AddBeacon(FVector loc, DWORD pattern, int type = 1);
	virtual void   SetPeriod(double p);
	virtual void   SetPattern(int index, DWORD p);
	virtual void   SetOffset(DWORD o);

	virtual void   Orient(const Physical* rep);

protected:
	double         period;
	double         scale;
	bool           enable;

	int            nlights;

	FVector        loc[MAX_LIGHTS];
	DriveSprite* beacon[MAX_LIGHTS];
	DWORD          pattern[MAX_LIGHTS];
	int            beacon_type[MAX_LIGHTS];
	DWORD          offset;
};

