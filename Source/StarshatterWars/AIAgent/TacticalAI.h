/*  Project STARSHATTER WARS
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios

	SUBSYSTEM:    Stars.exe
	FILE:         TacticalAI.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Common base class and interface for mid-level (tactical) AI
*/

#pragma once

#include "Types.h"
#include "SimDirector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class Ship;
class ShipAI;
class Instruction;
class CarrierAI;

// +--------------------------------------------------------------------+

class TacticalAI : public SimDirector
{
public:
	TacticalAI(ShipAI* ai);
	virtual ~TacticalAI();

	enum ROE {
		NONE,
		SELF_DEFENSIVE,
		DEFENSIVE,
		DIRECTED,
		FLEXIBLE,
		AGRESSIVE
	};

	virtual void      ExecFrame(double seconds);

	virtual ROE       RulesOfEngagement()  const { return roe; }
	virtual double    ThreatLevel()        const { return threat_level; }
	virtual double    SupportLevel()       const { return support_level; }

protected:
	// pick the best target if we don't have one yet:
	virtual void      CheckOrders();
	virtual bool      CheckShipOrders();
	virtual bool      ProcessOrders();
	virtual bool      CheckFlightPlan();
	virtual bool      CheckObjectives();

	virtual void      SelectTarget();
	virtual void      SelectTargetDirected(Ship* tgt = 0);
	virtual void      SelectTargetOpportunity();
	virtual void      CheckTarget();
	virtual void      FindThreat();
	virtual void      FindSupport();
	virtual void      FindFormationSlot(INSTRUCTION_FORMATION formation);

	virtual bool      CanTarget(Ship* tgt);
	virtual void      ClearRadioOrders();

	Ship* ship;
	ShipAI* ship_ai;
	CarrierAI* carrier_ai;

	Instruction* navpt;
	Instruction* orders;

	double				agression;
	ROE					roe;
	int					element_index;
	RadioMessageAction	action;
	int					exec_time;
	int					directed_tgtid;

	double				threat_level;
	double				support_level;
};
