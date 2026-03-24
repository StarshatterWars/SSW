/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         FlightComputer.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Flight Computer systems class
*/

#pragma once

#include "Types.h"
#include "Computer.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class FlightComputer : public Computer
{
public:
	enum CompType { AVIONICS = 1, FLIGHT, TACTICAL };

	FlightComputer(int comp_type, const char* comp_name);
	FlightComputer(const Computer& rhs);
	virtual ~FlightComputer();

	virtual void      ExecSubFrame();

	int               Mode()                  const { return mode; }
	double            Throttle()              const { return throttle; }

	void              SetMode(int m) { mode = m; }
	void              SetVelocityLimit(double v) { vlimit = (float)v; }
	void              SetTransLimit(double x, double y, double z);

	void              FullStop() { halt = true; }

protected:
	virtual void      ExecTrans();
	virtual void      ExecThrottle();

	int               mode;
	int               halt;
	float             throttle;

	float             vlimit;
	float             trans_x_limit;
	float             trans_y_limit;
	float             trans_z_limit;
};

