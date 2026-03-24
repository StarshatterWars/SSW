/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimComponent.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic ship system sub-component class
*/

#pragma once

#include "Types.h"
#include "Text.h"

#include "Math/Vector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class SimSystem;

// +--------------------------------------------------------------------+
// Design record (data-driven definition)

class ComponentDesign
{
public:
	static const char* TYPENAME() { return "ComponentDesign"; }

	ComponentDesign();
	~ComponentDesign();

	int operator == (const ComponentDesign& rhs) const { return (name == rhs.name); }

	// identification:
	Text   name;
	Text   abrv;

	float  repair_time;
	float  replace_time;
	int    spares;
	int    affects;
};

// +--------------------------------------------------------------------+
// Runtime instance (sim state)

class SimComponent
{
public:
	static const char* TYPENAME() { return "SimComponent"; }

	enum DAMAGE {
		DAMAGE_EFFICIENCY = 0x01,
		DAMAGE_SAFETY = 0x02,
		DAMAGE_STABILITY = 0x04
	};

	SimComponent(ComponentDesign* d, SimSystem* s);
	SimComponent(const SimComponent& c);
	virtual ~SimComponent();

	const char* Name()         const { return design->name; }
	const char* Abbreviation() const { return design->abrv; }
	float       RepairTime()   const { return design->repair_time; }
	float       ReplaceTime()  const { return design->replace_time; }

	bool DamageEfficiency() const { return (design->affects & DAMAGE_EFFICIENCY) ? true : false; }
	bool DamageSafety()     const { return (design->affects & DAMAGE_SAFETY) ? true : false; }
	bool DamageStability()  const { return (design->affects & DAMAGE_STABILITY) ? true : false; }

	SYSTEM_STATUS	GetStatus() const { return Status; }
	void			SetStatus(SYSTEM_STATUS stat) { Status = stat; }
	float  Availability()  const;
	float  TimeRemaining() const;
	int    SpareCount()    const;
	bool   IsJerried()     const;
	int    NumJerried()    const;

	void   SetSystem(SimSystem* s) { system = s; }
	SimSystem* GetSystem()   const { return system; }

	virtual void ApplyDamage(double damage);
	virtual void ExecMaintFrame(double seconds);
	virtual void Repair();
	virtual void Replace();

protected:
	ComponentDesign* design;

	// Component health status:
	SYSTEM_STATUS	Status;
	float			availability;
	float			time_remaining;
	int				spares;
	int				jerried;
	SimSystem*		system;
};
