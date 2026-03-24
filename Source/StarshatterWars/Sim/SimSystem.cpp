/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         SimSystem.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Generic Ship Subsystem class
*/

#include "SimSystem.h"
#include "SystemDesign.h"
#include "SimComponent.h"
#include "Game.h"

// Unreal logging (replaces Print):
#include "Logging/LogMacros.h"

// Minimal Unreal math (for FVector):
#include "Math/Vector.h"

// Define a local log category for this translation unit:
DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSystem, Log, All);

// +----------------------------------------------------------------------+

SimSystem::SimSystem(SYSTEM_CATEGORY t, int s, const char* n, int maxv,
	double e, double c, double r)
	: type(t),
	ship(0),
	id(0),
	subtype(s),
	max_value(0),
	Status(SYSTEM_STATUS::NOMINAL),
	crit_level(0.5f),
	availability(1.0f),
	safety(1.0f),
	stability(1.0f),
	safety_overload(0.0f),
	net_avail(-1.0f),
	mount_loc(FVector::ZeroVector),
	mount_rel(FVector::ZeroVector),
	radius(0.0f),
	hull_factor(0.5f),
	energy((float)e),
	capacity((float)c),
	sink_rate((float)r),
	power_level(1.0f),
	source_index(0),
	power_flags(0),
	power_on(true),
	emcon(3),
	explosion_type(0),
	design(0),
	components()
{
	name = n;
	abrv = name;

	if (maxv < 100)
		max_value = maxv;
	else
		max_value = (int)(maxv / 100.0);

	emcon_power[0] = 100;
	emcon_power[1] = 100;
	emcon_power[2] = 100;
}

// +----------------------------------------------------------------------+

SimSystem::SimSystem(const SimSystem& s)
	: type(s.type),
	ship(0),
	id(s.id),
	subtype(s.subtype),
	max_value(s.max_value),
	Status(s.Status),
	crit_level(s.crit_level),
	availability(s.availability),
	safety(s.safety),
	stability(s.stability),
	safety_overload(0.0f),
	net_avail(-1.0f),
	mount_loc(FVector::ZeroVector),
	mount_rel(s.mount_rel),
	radius(s.radius),
	hull_factor(s.hull_factor),
	energy(s.energy),
	capacity(s.capacity),
	sink_rate(s.sink_rate),
	power_level(s.power_level),
	source_index(s.source_index),
	power_flags(s.power_flags),
	power_on(s.power_on),
	emcon(s.emcon),
	explosion_type(s.explosion_type),
	design(s.design),
	components()
{
	name = s.name;
	abrv = s.abrv;

	if (design) {
		// cast-away const (matches original Starshatter behavior):
		ListIter<SimComponent> c = (List<SimComponent>&) s.components;
		while (++c) {
			SimComponent* comp = new SimComponent(*(c.value()));
			comp->SetSystem(this);
			components.append(comp);
		}
	}

	emcon_power[0] = s.emcon_power[0];
	emcon_power[1] = s.emcon_power[1];
	emcon_power[2] = s.emcon_power[2];
}

// +--------------------------------------------------------------------+

SimSystem::~SimSystem()
{
	components.destroy();
}

// +--------------------------------------------------------------------+

void
SimSystem::SetDesign(SystemDesign* d)
{
	if (design) {
		design = 0;
		components.destroy();
	}

	if (d) {
		design = d;

		ListIter<ComponentDesign> cd = design->components;
		while (++cd) {
			SimComponent* comp = new SimComponent(cd.value(), this);
			components.append(comp);
		}
	}
}

// +--------------------------------------------------------------------+

void
SimSystem::SetPowerLevel(double level)
{
	if (level > 100)
		level = 100;
	else if (level < 0)
		level = 0;

	level /= 100;

	if (power_level != level) {
		// if the system is on emergency override power,
		// do notxlet the EMCON system use this method
		// to drop it back to normal power:
		if (power_level > 1 && level == 1)
			return;

		power_level = (float)level;

		//NetUtil::SendSysStatus(ship, this);
	}
}

void
SimSystem::SetOverride(bool over)
{
	bool changed = false;

	if (over && power_level != 1.2f) {
		power_level = 1.2f;
		changed = true;
	}

	else if (!over && power_level > 1) {
		power_level = 1.0f;
		changed = true;
	}

	if (changed) {
		//NetUtil::SendSysStatus(ship, this);
	}	
}

void
SimSystem::SetEMCONPower(int index, int InPowerLevel)
{
	if (index >= 1 && index <= 3) {
		emcon_power[index - 1] = (BYTE)InPowerLevel;
	}
}

int
SimSystem::GetEMCONPower(int index)
{
	if (index >= 1 && index <= 3) {
		return emcon_power[index - 1];
	}

	return 100;
}

void
SimSystem::DoEMCON(int index)
{
	int e = GetEMCONPower(index);

	if (power_level * 100 > e || emcon != index) {
		if (e == 0) {
			PowerOff();
		}
		else {
			if (emcon != index)
				PowerOn();

			SetPowerLevel(e);
		}
	}

	emcon = (BYTE)index;
}

// +--------------------------------------------------------------------+

void
SimSystem::ExecFrame(double seconds)
{
	if (seconds < 0.01)
		seconds = 0.01;

	SYSTEM_STATUS s = SYSTEM_STATUS::DESTROYED;

	if (availability > 0.99)
		s = SYSTEM_STATUS::NOMINAL;
	else if (availability > crit_level)
		s = SYSTEM_STATUS::DEGRADED;
	else
		s = SYSTEM_STATUS::CRITICAL;

	bool repair = false;

	if (components.size() > 0) {
		ListIter<SimComponent> comp = components;
		while (++comp) {
			if (comp->GetStatus() > SYSTEM_STATUS::NOMINAL) {
				repair = true;
				break;
			}
		}
	}

	if (repair) {
		Repair();
	}

	else {
		if (Status != s) {
			Status = s;
			//NetUtil::SendSysStatus(ship, this);
		}

		// collateral damage due to unsafe operation:
		if (power_on && power_level > safety) {
			safety_overload += (float)seconds;

			// inflict some damage now:
			if (safety_overload > 60) {
				// Keep behavior similar to original rand() expression:
				const float Denom = (float)(1000.0 * (power_level - safety));
				const float Delta = (Denom > 0.0f) ? ((float)rand() / Denom) : 0.0f;

				safety_overload -= Delta;
				ApplyDamage(15);

				//NetUtil::SendSysStatus(ship, this);
			}
		}

		else if (safety_overload > 0) {
			safety_overload -= (float)seconds;
		}
	}
}

void
SimSystem::Repair()
{
	if (Status != SYSTEM_STATUS::MAINT) {
		Status = SYSTEM_STATUS::MAINT;
		safety_overload = 0.0f;

		//NetUtil::SendSysStatus(ship, this);
	}
}

// +--------------------------------------------------------------------+

void
SimSystem::ExecMaintFrame(double seconds)
{
	if (components.size() > 0) {
		ListIter<SimComponent> comp = components;
		while (++comp) {
			if (comp->GetStatus() > SYSTEM_STATUS::NOMINAL) {
				comp->ExecMaintFrame(seconds);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
SimSystem::ApplyDamage(double damage)
{
	if (!power_on)
		damage /= 10;

	if (components.size() > 0) {
		int index = rand() % components.size();

		if (damage > 50) {
			damage /= 2;
			components[index]->ApplyDamage(damage);

			index = rand() % components.size();
		}

		components[index]->ApplyDamage(damage);

		if (safety < 0.5)
			SetPowerLevel(50);

		else if (safety < 1.0)
			SetPowerLevel(safety * 100);
	}
	else {
		availability -= (float)(damage / 100.0f);
		if (availability < 0.01) availability = 0.0f;
	}
}

void
SimSystem::CalcStatus()
{
	if (components.size() > 0) {
		availability = 1.0f;
		safety = 1.0f;
		stability = 1.0f;

		ListIter<SimComponent> comp = components;
		while (++comp) {
			if (comp->DamageEfficiency())
				availability *= comp->Availability() / 100.0f;

			if (comp->DamageSafety())
				safety *= comp->Availability() / 100.0f;

			if (comp->DamageStability())
				stability *= comp->Availability() / 100.0f;

			if (comp->IsJerried()) {
				safety *= 0.95f;
				stability *= 0.95f;
			}
		}

		if (net_avail >= 0 && availability < net_avail)
			availability = net_avail;
	}
}

// +--------------------------------------------------------------------+

void
SimSystem::Mount(FVector InLoc, float rad, float hull)
{
	mount_rel = InLoc;
	radius = rad;
	hull_factor = hull;
}

void
SimSystem::Mount(const SimSystem& system)
{
	mount_rel = system.mount_rel;
	radius = system.radius;
	hull_factor = system.hull_factor;
}

// +--------------------------------------------------------------------+

void
SimSystem::Orient(const Physical* rep)
{
	// Starshatter core types:
	const Matrix& StarOrient = rep->Cam().Orientation();
	const FVector Loc = rep->Location();

	// Convert Starshatter Matrix -> Unreal FMatrix
	FMatrix UEOrient = FMatrix::Identity;
	UEOrient.M[0][0] = (float)StarOrient(0, 0);  UEOrient.M[0][1] = (float)StarOrient(0, 1);  UEOrient.M[0][2] = (float)StarOrient(0, 2);
	UEOrient.M[1][0] = (float)StarOrient(1, 0);  UEOrient.M[1][1] = (float)StarOrient(1, 1);  UEOrient.M[1][2] = (float)StarOrient(1, 2);
	UEOrient.M[2][0] = (float)StarOrient(2, 0);  UEOrient.M[2][1] = (float)StarOrient(2, 1);  UEOrient.M[2][2] = (float)StarOrient(2, 2);

	// Local mount offset (already in your struct, now treated as FVector)
	const FVector MountRelUE(mount_rel.X, mount_rel.Y, mount_rel.Z);

	// Rotate local offset, then translate into world space
	const FVector SsMount = UEOrient.TransformVector(MountRelUE) + Loc;

	// Store (mount_loc is FVector in your port)
	mount_loc = SsMount;
}

// +----------------------------------------------------------------------+

double
SimSystem::GetRequest(double seconds) const
{
	if (!power_on || capacity == energy)
		return 0;

	else
		return power_level * sink_rate * seconds;
}

// +----------------------------------------------------------------------+

void
SimSystem::Distribute(double delivered_energy, double seconds)
{
	if (UsesWatts()) {
		if (seconds < 0.01)
			seconds = 0.01;

		// convert Joules to Watts:
		energy = (float)(delivered_energy / seconds);
	}

	else if (!Game::Paused()) {
		energy += (float)delivered_energy;

		if (energy > capacity)
			energy = capacity;

		else if (energy < 0)
			energy = 0.0f;
	}
}

// +----------------------------------------------------------------------+

void
SimSystem::DrainPower(double to_level)
{
	energy = 0.0f;

	// Converted from Print-style debugging:
	UE_LOG(LogStarshatterSystem, Verbose, TEXT("SimSystem::DrainPower to_level=%f (ignored; energy set to 0)"), (double)to_level);
}

