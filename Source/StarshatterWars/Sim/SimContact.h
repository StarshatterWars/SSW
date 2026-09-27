/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimContact.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Sensor Contact class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "SimSystem.h"

#include "Math/Vector.h" // FVector

// +--------------------------------------------------------------------+

class Ship;
class SimShot;

class SimContact : public SimObserver
{
	friend class Sensor;

public:
	static const char* TYPENAME() { return "SimContact"; }

	SimContact();
	SimContact(Ship* s, float p, float a);
	SimContact(SimShot* s, float p, float a);
	virtual ~SimContact();

	int operator == (const SimContact& c) const;

	Ship*    GetShip()   const { return ship; }
	SimShot* GetShot()   const { return shot; }
	FVector  Location()  const { return loc; }

	double   PasReturn() const { return d_pas; }
	double   ActReturn() const { return d_act; }
	bool     PasLock()   const;
	bool     ActLock()   const;
	double   Age()       const;
	bool     IsProbed()  const { return probe; }

	DWORD    AcquisitionTime() const { return acquire_time; }

	int      GetIFF(const Ship* observer) const;
	void     GetBearing(const Ship* observer, double& az, double& el, double& r) const;
	double   Range(const Ship* observer, double limit = 75e3) const;

	bool     InFront(const Ship* observer) const;
	bool     Threat(const Ship* observer)  const;
	bool     Visible(const Ship* observer) const;

	void     Reset();
	void     Merge(SimContact* c);
	void     ClearTrack();
	void     UpdateTrack();
	int      TrackLength() const { return ntrack; }
	FVector  TrackPoint(int i) const;

	virtual bool         Update(SimObject* obj);
	virtual const char* GetObserverName() const;

private:
	Ship* ship = nullptr;
	SimShot* shot = nullptr;
	FVector  loc = FVector::ZeroVector;

	DWORD    acquire_time = 0;
	DWORD    time = 0;

	FVector* track = nullptr;
	int      ntrack = 0;
	DWORD    track_time = 0;

	float    d_pas = 0.0f;  // power output
	float    d_act = 0.0f;  // mass, size
	bool     probe = false; // scanned by probe
};
