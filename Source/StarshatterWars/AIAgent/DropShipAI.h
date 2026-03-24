/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         DropShipAI.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Drop Ship (orbit/surface and surface/orbit) AI class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "ShipAI.h"

// Forward declarations:
class Ship;

class DropShipAI : public ShipAI
{
public:
	explicit DropShipAI(Ship* s);
	virtual ~DropShipAI();

	enum { DIR_TYPE = 2001 };
	virtual int Type() const override { return DIR_TYPE; }

protected:
	// Accumulate behaviors:
	virtual void Navigator() override;
	virtual void FindObjective() override;
};
