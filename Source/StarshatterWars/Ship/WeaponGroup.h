/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         WeaponGroup.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Weapon Control Category (Group) class
*/

#pragma once

#include "CoreMinimal.h"
#include "Types.h"
#include "Weapon.h"
#include "Text.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class SimObject;
class System;

// +--------------------------------------------------------------------+

class WeaponGroup
{
public:
	static const char* TYPENAME() { return "WeaponGroup"; }

	WeaponGroup(const char* name);
	~WeaponGroup();

	void              ExecFrame(double factor);

	// identification:
	const char*		  Name()                     const { return name; }
	const char*		  Abbreviation()             const { return abrv; }
	void              SetName(const char* n);
	void              SetAbbreviation(const char* a);

	bool              IsPrimary()                const;
	bool              IsDrone()                  const;
	bool              IsDecoy()                  const;
	bool              IsProbe()                  const;
	bool              IsMissile()                const;
	bool              IsBeam()                   const;
	int               Value()                    const;

	// weapon list:
	void              AddWeapon(Weapon* w);
	int               NumWeapons()               const;
	List<Weapon>&	  GetWeapons();
	bool              Contains(const Weapon* w)  const;

	// weapon selection:
	void              SelectWeapon(int n);
	void              CycleWeapon();
	Weapon*			  GetWeapon(int n)           const;
	Weapon*			  GetSelected()              const;

	// operations:
	bool              GetTrigger()               const { return trigger; }
	void              SetTrigger(bool t = true) { trigger = t; }
	int               Ammo()                     const { return ammo; }
	float             Mass()                     const { return mass; }
	float             Resistance()               const { return resist; }
	void              CheckAmmo();

	void              SetTarget(SimObject* t, SimSystem* sub = 0);
	SimObject*		  GetTarget()                const;
	SimSystem*		  GetSubTarget()             const;
	void              DropTarget();
	void              SetFiringOrders(WeaponsOrders o);
	WeaponsOrders     GetFiringOrders()          const { return orders; }
	void              SetControlMode(WeaponsControl m);
	WeaponsControl    GetControlMode()           const { return control; }
	void              SetSweep(WeaponsSweep s);
	WeaponsSweep      GetSweep()                 const { return sweep; }
	SYSTEM_STATUS     GetStatus()                const;

	WeaponDesign*	  GetDesign()                const;
	bool              CanTarget(uint32 tgt_class) const;

	void              PowerOn();
	void              PowerOff();

protected:
	// Displayable name:
	Text              name;
	Text              abrv;

	List<Weapon>      weapons;

	int               selected;
	bool              trigger;
	int               ammo;

	WeaponsOrders     orders;
	WeaponsControl    control;
	WeaponsSweep      sweep;

	float             mass;
	float             resist;
};
