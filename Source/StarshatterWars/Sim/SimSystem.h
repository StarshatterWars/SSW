/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimSystem.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic ship System class
*/

#pragma once

#include "Types.h"
#include "Physical.h"
#include "List.h"
#include "Text.h"
#include "GameStructs.h"

// Minimal Unreal include (replaces Point/Vec3):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class SimComponent;
class Ship;
class SystemDesign;

// +--------------------------------------------------------------------+

class SimSystem
{
	friend SimComponent;

public:
	static const char* TYPENAME() { return "SimSystem"; }

	enum CATEGORY {
		MISC_SYSTEM = 0, DRIVE = 1, WEAPON, SHIELD, SENSOR,
		COMPUTER, POWER_SOURCE, FLIGHT_DECK, FARCASTER
	};

	enum POWER_FLAGS { POWER_WATTS = 1, POWER_CRITICAL = 2 };

	SimSystem(SYSTEM_CATEGORY t, int s, const char* n, int maxv,
		double energy = 0, double capacity = 100, double sink_rate = 1);
	SimSystem(const SimSystem& s);
	virtual ~SimSystem();

	int operator==(const SimSystem& s) const { return this == &s; }

	SYSTEM_CATEGORY			GetType()      const { return type; }
	int						Subtype()      const { return subtype; }
	const char*				Name()         const { return name; }
	const char*				Abbreviation() const { return abrv; }
	SYSTEM_STATUS			GetStatus() const { return Status; }

	void              SetName(const char* n) { name = n; }
	void              SetAbbreviation(const char* a) { abrv = a; }
	void              SetDesign(SystemDesign* d);

	virtual int       Value()        const { return (int)(max_value * availability * 100); }
	int               MaxValue()     const { return (int)(max_value * 100); }

	double            Availability() const { return       availability * 100; }
	double            Safety()       const { return       safety * 100; }
	double            Stability()    const { return       stability * 100; }
	virtual void      CalcStatus();
	virtual void      Repair();

	double            NetAvail()      const { return net_avail; }
	void              SetNetAvail(double d) { net_avail = (float)d; }

	List<SimComponent>& GetComponents() { return components; }

	virtual void      ApplyDamage(double damage);
	virtual void      ExecFrame(double seconds);
	virtual void      ExecMaintFrame(double seconds);
	virtual void      DoEMCON(int emcon);

	// PHYSICAL LOCATION (for inflicting system damage):
	virtual void      Orient(const Physical* rep);
	virtual void      Mount(FVector InLoc, float InRadius, float InHullFactor = 0.5f);
	virtual void      Mount(const SimSystem& system);

	FVector           MountLocation()  const { return mount_loc; }
	double            Radius()         const { return radius; }
	double            HullProtection() const { return hull_factor; }

	// POWER UTILIZATION:
	bool              IsPowerCritical() const { return (power_flags & POWER_CRITICAL) ? true : false; }
	bool              UsesWatts()       const { return (power_flags & POWER_WATTS) ? true : false; }

	virtual double    GetRequest(double seconds) const;
	virtual void      Distribute(double delivered_energy, double seconds);

	int               GetSourceIndex() const { return source_index; }
	void              SetSourceIndex(int i) { source_index = i; }

	virtual int       Charge()         const { return (int)(100 * energy / capacity); }

	bool              IsPowerOn()       const { return power_on; }
	virtual void      PowerOn() { power_on = true; }
	virtual void      PowerOff() { power_on = false; }

	// percentage, but stored as 0-1
	virtual double    GetPowerLevel()   const { return power_level * 100; }
	virtual void      SetPowerLevel(double level);
	virtual void      SetOverride(bool over);

	// for power drain damage:
	virtual void      DrainPower(double to_level);

	void              SetCapacity(double c) { capacity = (float)c; }
	double            GetCapacity()   const { return capacity; }
	double            GetEnergy()     const { return energy; }
	double            GetSinkRate()   const { return sink_rate; }
	void              SetEMCONPower(int emcon, int power_level);
	int               GetEMCONPower(int emcon);

	int               GetExplosionType() const { return explosion_type; }
	void              SetExplosionType(int t) { explosion_type = t; }

	Ship*			  GetShip() const { return ship; }
	void              SetShip(Ship* s) { ship = s; }
	int               GetID()        const { return id; }
	void              SetID(int n) { id = n; }

protected:
	// AI information:
	SYSTEM_CATEGORY   type;
	Ship*			  ship;
	int               id;
	int               subtype;
	int               max_value;

	// Displayable name:
	Text              name;
	Text              abrv;

	// System health status:
	SYSTEM_STATUS     Status;
	float             crit_level;
	float             availability;
	float             safety;
	float             stability;
	float             safety_overload;
	float             net_avail;

	// Mounting:
	FVector           mount_loc;  // world space
	FVector           mount_rel;  // object space
	float             radius;
	float             hull_factor;

	// Power Sink:
	float             energy;
	float             capacity;
	float             sink_rate;
	float             power_level;
	int               source_index;
	DWORD             power_flags;
	bool              power_on;
	BYTE              emcon_power[3];
	BYTE              emcon;

	int               explosion_type;

	// Subcomponents:
	SystemDesign* design;
	List<SimComponent>  components;
};

