/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         FighterTacticalAI.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Fighter-specific mid-level (tactical) AI
*/

#pragma once

#include "Types.h"
#include "TacticalAI.h"
#include "List.h"

// Minimal Unreal include required for FVector usage patterns elsewhere;
// also provides UE integral types (uint32, etc.) if needed.
#include "Math/Vector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class WeaponGroup;

// +--------------------------------------------------------------------+

class FighterTacticalAI : public TacticalAI
{
public:
	FighterTacticalAI(ShipAI* ai);
	virtual ~FighterTacticalAI();

protected:
	virtual bool      CheckFlightPlan();
	virtual bool      IsStrikeComplete(Instruction* instr = 0);

	virtual void      SelectTarget();
	virtual void      SelectTargetDirected(Ship* tgt = 0);
	virtual void      SelectTargetOpportunity();
	virtual void      FindFormationSlot(INSTRUCTION_FORMATION formation);
	virtual void      FindThreat();

	virtual void      SelectSecondaryForTarget(Ship* tgt);
	virtual int       ListSecondariesForTarget(Ship* tgt, List<WeaponGroup>& weps);

	bool              winchester[4];

	// UE-compatible timing types (replace legacy DWORD):
	uint32            THREAT_REACTION_TIME;
	uint32            secondary_selection_time;

	int               ai_level;
};

// +--------------------------------------------------------------------+

