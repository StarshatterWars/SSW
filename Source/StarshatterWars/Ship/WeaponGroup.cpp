/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         WeaponGroup.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Weapon Control Category (Group) class
*/

#include "WeaponGroup.h"
#include "Ship.h"
#include "CoreMinimal.h"
#include "GameStructs.h"

// +----------------------------------------------------------------------+

WeaponGroup::WeaponGroup(const char* n)
	: selected(0)
	, trigger(false)
	, orders(WeaponsOrders::MANUAL)
	, control(WeaponsControl::SINGLE_FIRE)
	, sweep(WeaponsSweep::SWEEP_TIGHT)
	, mass(0.0f)
	, resist(0.0f)
	, name(n)
	, ammo(0)
{
}

// +--------------------------------------------------------------------+

WeaponGroup::~WeaponGroup()
{
	weapons.destroy();
}

// +--------------------------------------------------------------------+

void WeaponGroup::SetName(const char* n)
{
	name = n;
}

void WeaponGroup::SetAbbreviation(const char* a)
{
	abrv = a;
}

// +--------------------------------------------------------------------+

bool WeaponGroup::IsPrimary() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsPrimary();

	return false;
}

bool WeaponGroup::IsDrone() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsDrone();

	return false;
}

bool WeaponGroup::IsDecoy() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsDecoy();

	return false;
}

bool WeaponGroup::IsProbe() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsProbe();

	return false;
}

bool WeaponGroup::IsMissile() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsMissile();

	return false;
}

bool WeaponGroup::IsBeam() const
{
	if (weapons.size() > 0)
		return weapons[0]->IsBeam();

	return false;
}

// +--------------------------------------------------------------------+

void WeaponGroup::AddWeapon(Weapon* w)
{
	weapons.append(w);
}

int WeaponGroup::NumWeapons() const
{
	return weapons.size();
}

List<Weapon>& WeaponGroup::GetWeapons()
{
	return weapons;
}

bool WeaponGroup::Contains(const Weapon* w) const
{
	return weapons.contains(w) ? true : false;
}

// +--------------------------------------------------------------------+

void WeaponGroup::SelectWeapon(int n)
{
	if (n >= 0 && n < weapons.size())
		selected = n;
}

void WeaponGroup::CycleWeapon()
{
	selected++;

	if (selected >= weapons.size())
		selected = 0;
}

Weapon* WeaponGroup::GetWeapon(int n) const
{
	if (n >= 0 && n < weapons.size())
		return weapons[n];

	return 0;
}

Weapon* WeaponGroup::GetSelected() const
{
	return weapons[selected];
}

bool WeaponGroup::CanTarget(uint32 tgt_class) const
{
	if (selected >= 0 && selected < weapons.size())
		return weapons[selected]->CanTarget(tgt_class);

	return false;
}

// +--------------------------------------------------------------------+

void WeaponGroup::ExecFrame(double seconds)
{
	ammo = 0;
	mass = 0.0f;
	resist = 0.0f;

	ListIter<Weapon> iter = weapons;
	while (++iter) {
		Weapon* w = iter.value();
		w->ExecFrame(seconds);

		ammo += w->Ammo();
		mass += w->Mass();
		resist += w->Resistance();
	}
}

void WeaponGroup::CheckAmmo()
{
	ammo = 0;
	mass = 0.0f;
	resist = 0.0f;

	ListIter<Weapon> iter = weapons;
	while (++iter) {
		Weapon* w = iter.value();

		ammo += w->Ammo();
		mass += w->Mass();
		resist += w->Resistance();
	}
}

// +--------------------------------------------------------------------+

void WeaponGroup::SetTarget(SimObject* target, SimSystem* subtarget)
{
	ListIter<Weapon> w = weapons;
	while (++w)
		w->SetTarget(target, subtarget);
}

SimObject* WeaponGroup::GetTarget() const
{
	SimObject* target = 0;

	if (weapons.size())
		target = weapons[0]->GetTarget();

	return target;
}

SimSystem* WeaponGroup::GetSubTarget() const
{
	SimSystem* subtarget = 0;

	if (weapons.size())
		subtarget = weapons[0]->GetSubTarget();

	return subtarget;
}

void WeaponGroup::DropTarget()
{
	ListIter<Weapon> w = weapons;
	while (++w)
		w->SetTarget(0, 0);
}

// +--------------------------------------------------------------------+

WeaponDesign* WeaponGroup::GetDesign() const
{
	if (selected >= 0 && selected < weapons.size())
		return (WeaponDesign*)weapons[selected]->Design();

	return 0;
}

// +--------------------------------------------------------------------+

SYSTEM_STATUS WeaponGroup::GetStatus() const
{
	SYSTEM_STATUS status = SYSTEM_STATUS::NOMINAL;
	int critical = true;

	ListIter<Weapon> iter = (List<Weapon>&)weapons; // cast-away const
	while (++iter) {
		Weapon* w = iter.value();

		if (w->GetStatus() < SYSTEM_STATUS::NOMINAL)
			status = SYSTEM_STATUS::DEGRADED;

		if (w->GetStatus() > SYSTEM_STATUS::CRITICAL)
			critical = false;
	}

	if (critical)
		return SYSTEM_STATUS::CRITICAL;

	return status;
}

// +--------------------------------------------------------------------+

void WeaponGroup::SetFiringOrders(WeaponsOrders o)
{
	orders = o;

	ListIter<Weapon> w = weapons;
	while (++w)
		w->SetFiringOrders(orders);
}

void WeaponGroup::SetControlMode(WeaponsControl m)
{
	control = m;

	ListIter<Weapon> w = weapons;
	while (++w)
		w->SetControlMode(control);
}

void WeaponGroup::SetSweep(WeaponsSweep s)
{
	sweep = s;

	ListIter<Weapon> w = weapons;
	while (++w)
		w->SetSweep(sweep);
}

// +--------------------------------------------------------------------+

void WeaponGroup::PowerOff()
{
	ListIter<Weapon> w = weapons;
	while (++w)
		w->PowerOff();
}

void WeaponGroup::PowerOn()
{
	ListIter<Weapon> w = weapons;
	while (++w)
		w->PowerOn();
}

// +--------------------------------------------------------------------+

int WeaponGroup::Value() const
{
	int result = 0;

	for (int i = 0; i < weapons.size(); i++) {
		const Weapon* w = weapons[i];
		result += w->Value();
	}

	return result;
}
