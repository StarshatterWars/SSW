/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Computer.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Various computer systems class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"

// No FVector usage in this header; keep Unreal includes out for minimal compile surface.

class Computer : public SimSystem
{
public:
	enum CompType { AVIONICS = 1, FLIGHT, TACTICAL };

	Computer(int comp_type, const char* comp_name);
	Computer(const Computer& rhs);
	virtual ~Computer();

	virtual void      ApplyDamage(double damage);
	virtual void      ExecFrame(double seconds);
	virtual void      Distribute(double delivered_energy, double seconds);

protected:
};
