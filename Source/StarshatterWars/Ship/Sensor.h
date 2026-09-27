/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Sensor.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	OVERVIEW
	========
	Integrated (Passive and Active) Sensor Package class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "SimSystem.h"
#include "Geometry.h"
#include "List.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

// Minimal Unreal includes (required by API surface):
#include "Math/Vector.h" // FVector

// +--------------------------------------------------------------------+

class SimShot;
class SimContact;
class Ship;

// +--------------------------------------------------------------------+

class Sensor : public SimSystem, public SimObserver
{
public:
	Sensor();
	Sensor(const Sensor& rhs);
	virtual ~Sensor();

	virtual void       ExecFrame(double seconds);
	virtual SimObject* LockTarget(ESimObject obj_type = ESimObject::SHIP,
		bool closest = false,
		bool hostile = false);
	virtual SimObject* LockTarget(SimObject* candidate);
	virtual bool       IsTracking(SimObject* tgt);
	virtual void       DoEMCON(int emcon);

	virtual void       ClearAllContacts();

	virtual ESensorMode  GetMode() const { return mode; }
	virtual void       SetMode(ESensorMode m);
	
	virtual double     GetBeamLimit() const;
	virtual double     GetBeamRange() const;
	virtual void       IncreaseRange();
	virtual void       DecreaseRange();
	virtual void       AddRange(double r);

	SimContact* FindContact(Ship* s);
	SimContact* FindContact(SimShot* s);

	// borrow this sensor for missile seeker
	SimObject* AcquirePassiveTargetForMissile();
	SimObject* AcquireActiveTargetForMissile();

	void SetRangeSettings(const TArray<float>& InRanges);

	static ESensorMode GetNextSensorMode(ESensorMode Mode);

	void SetShip(Ship* s) { ship = s; }
	Ship* GetShip() const { return ship; }

	// SimObserver:
	virtual bool        Update(SimObject* obj);
	virtual const char* GetObserverName() const;

protected:
	void              ProcessContact(Ship* contact, double az1, double az2);
	void              ProcessContact(SimShot* contact, double az1, double az2);

	ESensorMode       mode;
	int               nsettings;
	int               range_index;
	float             range_settings[8];
	SimObject* target;
	Ship* ship = nullptr;

	List<SimContact>     contacts;
};
