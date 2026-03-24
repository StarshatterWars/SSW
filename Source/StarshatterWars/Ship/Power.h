/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         Power.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Power generation and usage classes
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "List.h"

// +--------------------------------------------------------------------+

class PowerSource : public SimSystem
{
public:
	enum SUBTYPE { BATTERY, AUX, FUSION };

	PowerSource(SUBTYPE s, double max_output, double fuel_ratio = 0);
	PowerSource(const PowerSource& rhs);

	virtual void ExecFrame(double seconds);

	void AddClient(SimSystem* client);
	void RemoveClient(SimSystem* client);

	List<SimSystem>& Clients() { return clients; }

	virtual int    Charge() const;

	virtual void   SetFuelRange(double hours);

	bool           RouteChanged() const { return route_changed; }
	void           RouteScanned() { route_changed = false; }

	// override from System:
	virtual void   SetPowerLevel(double level);
	virtual void   SetOverride(bool over);

	// for power drain damage:
	virtual void   DrainPower(double to_level);

protected:
	float				max_output;
	float				fuel_ratio;
	List<SimSystem>		clients;
	bool				route_changed;
	float				requested_power_level;
};
