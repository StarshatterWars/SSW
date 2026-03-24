/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimObject.h
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Simulation Object and Observer classes
*/
#pragma once
#include "Types.h"
#include "Physical.h"
#include "List.h"

// +--------------------------------------------------------------------+
// Forward declarations (keep header light)

class UTexture2D;

class Sim;
class SimRegion;
class SimObserver;
class SimScene;

// +--------------------------------------------------------------------+

class SimObject : public Physical
{
	friend class SimRegion;

public:
	static const char* TYPENAME() { return "SimObject"; }

	enum TYPES {
		SIM_SHIP = 100,
		SIM_SHOT,
		SIM_DRONE,
		SIM_EXPLOSION,
		SIM_DEBRIS,
		SIM_ASTEROID
	};

	SimObject() : region(0), objid(0), active(0), notifying(0) {}
	SimObject(const char* n, int t = 0) : Physical(n, t), region(0), objid(0), active(0), notifying(0) {}
	virtual ~SimObject();

	virtual SimRegion*	 GetRegion()                 const { return region; }
	virtual void         SetRegion(SimRegion* rgn) { region = rgn; }

	virtual void         Notify();
	virtual void         Register(SimObserver* obs);
	virtual void         Unregister(SimObserver* obs);

	virtual void         Activate(SimScene& scene);
	virtual void         Deactivate(SimScene& scene);

	virtual DWORD        GetObjID() const { return objid; }
	virtual void         SetObjID(DWORD oid) { objid = oid; }

	virtual bool         IsHostileTo(const SimObject* o) const
	{
		return false;
	}

protected:
	SimRegion* region;
	List<SimObserver>    observers;
	DWORD                objid;
	bool                 active;
	bool                 notifying;
};

// +--------------------------------------------------------------------+

class SimObserver
{
public:
	static const char* TYPENAME() { return "SimObserver"; }

	virtual ~SimObserver();

	int operator == (const SimObserver& o) const { return this == &o; }

	virtual bool			Update(SimObject* obj);
	virtual const char*		GetObserverName() const;

	virtual void			Observe(SimObject* obj);
	virtual void			Ignore(SimObject* obj);

protected:
	List<SimObject>			observe_list;
};

