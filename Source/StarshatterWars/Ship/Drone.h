/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Drone.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Decoy / Weapons Drone class
*/

#pragma once

#include "Types.h"
#include "SimShot.h"

// Minimal Unreal include needed for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Camera;
class Ship;
class Trail;
class SimSystem;
class WeaponDesign;
class Sprite3D;
class USound;

// +--------------------------------------------------------------------+

class Drone : public SimShot
{
public:
	static const char* TYPENAME() { return "Drone"; }

	Drone(const FVector& pos, const Camera& cam, WeaponDesign* design, const Ship* ship = 0);
	virtual ~Drone();

	virtual void SeekTarget(SimObject* target, SimSystem* sub = 0) override;
	virtual void ExecFrame(double factor) override;

	virtual bool IsDrone() const override { return true; }
	virtual bool IsDecoy() const override { return decoy_type != 0; }
	virtual bool IsProbe() const override { return probe ? true : false; }

	virtual void Disarm() override;
	virtual void Destroy() override;

	// SENSORS AND VISIBILITY:
	virtual double      PCS() const override;
	virtual double      ACS() const override;
	virtual const char* ClassName() const;
	virtual int         Class() const;

	// DAMAGE RESOLUTION:
	void        SetLife(int seconds) { life = seconds; }
	virtual int HitBy(SimShot* shot, FVector& impact);

protected:
	int iff_code;
	int decoy_type;
	int probe;
};
