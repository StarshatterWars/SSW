/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Instruction.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Instruction (NavPoint / Order / Objective) class declaration
*/

#pragma once
#include "CoreMinimal.h"
#include "Types.h"
#include "SimObject.h"
#include "Text.h"
#include "RLoc.h"

// Minimal Unreal include (required for by-value FVector in the public API):
#include "Math/Vector.h"
#include "Math/Color.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class Instruction : public SimObserver
{
public:
	static const char* TYPENAME() { return "Instruction"; }

	enum PRIORITY
	{
		PRIMARY = 1,
		SECONDARY,
		BONUS
	};

	Instruction(INSTRUCTION_ACTION action, const char* tgt);
	Instruction(const char* rgn, FVector loc, INSTRUCTION_ACTION act = INSTRUCTION_ACTION::VECTOR);
	Instruction(SimRegion* rgn, FVector loc, INSTRUCTION_ACTION act = INSTRUCTION_ACTION::VECTOR);
	Instruction(const Instruction& instr);
	virtual ~Instruction();

	Instruction& operator = (const Instruction& n);
	static const char* ActionName(int ActionIndex);

	// accessors:
	static const char* ActionName(INSTRUCTION_ACTION a);

	static const char* StatusName(INSTRUCTION_STATUS s);
	static const char* FormationName(INSTRUCTION_FORMATION f);
	static const char* PriorityName(int p);

	const char* RegionName()  const { return rgn_name; }
	SimRegion* Region()      const { return region; }
	FVector      Location()    const;
	RLoc& GetRLoc() { return rloc; }

	INSTRUCTION_ACTION          GetAction()      const { return action; }
	INSTRUCTION_STATUS			GetStatus()      const { return status; }
	INSTRUCTION_FORMATION       GetFormation()   const { return formation; }

	RadioMessageAction          GetRadioAction()  const { return RadioAction; }

	int          Speed()       const { return speed; }
	int          EMCON()       const { return emcon; }
	int          WeaponsFree() const { return wep_free; }
	int          Priority()    const { return priority; }
	int          Farcast()     const { return farcast; }
	double       HoldTime()    const { return hold_time; }

	const char* TargetName() const { return tgt_name; }
	const char* TargetDesc() const { return tgt_desc; }
	SimObject*	GetTarget();

	void         Evaluate(Ship* s);
	const char* GetShortDescription() const;
	const char* GetDescription() const;

	// mutators:
	void         SetRegion(SimRegion* r) { region = r; }
	void         SetLocation(const FVector& l);
	void         SetAction(INSTRUCTION_ACTION s) { action = s; }
	void         SetStatus(INSTRUCTION_STATUS s);
	void         SetFormation(INSTRUCTION_FORMATION s) { formation = s; }

	void         SetRadioAction(RadioMessageAction ra) { RadioAction = ra; }
	void         SetSpeed(int s) { speed = s; }
	void         SetEMCON(int e) { emcon = e; }
	void         SetWeaponsFree(int f) { wep_free = f; }
	void         SetPriority(int p) { priority = p; }
	void         SetFarcast(int f) { farcast = f; }
	void         SetHoldTime(double t) { hold_time = t; }

	void		 SetTarget(const FString& InTarget); 
	void         SetTarget(SimObject* s);
	void         SetTargetDesc(const char* d);
	void         ClearTarget();

	virtual bool        Update(SimObject* s);
	virtual const char* GetObserverName() const;

protected:
	Text       rgn_name;
	SimRegion* region;
	RLoc       rloc;
	INSTRUCTION_ACTION		action;
	INSTRUCTION_FORMATION	formation;
	INSTRUCTION_STATUS      status;

	RadioMessageAction		RadioAction;
	
	int        speed;

	Text       tgt_name;
	Text       tgt_desc;
	SimObject* target;
	int        emcon;
	int        wep_free;
	int        priority;
	int        farcast;

	double     hold_time;
};

// +--------------------------------------------------------------------+


