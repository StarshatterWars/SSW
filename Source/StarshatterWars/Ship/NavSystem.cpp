/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	Original Author and Studio: John DiCamillo, Destroyer Studios LLC
	SUBSYSTEM:    Stars.exe
	FILE:         NavSystem.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Navigation System class implementation
*/

#include "NavSystem.h"

#include "Ship.h"
#include "Sim.h"
#include "HUDSounds.h"
#include "UIButton.h"
#include "Game.h"
#include "Instruction.h"
#include "FlightDeck.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogNavSystem, Log, All);

// +----------------------------------------------------------------------+

NavSystem::NavSystem()
	: SimSystem(SYSTEM_CATEGORY::COMPUTER, 2, "Auto Nav System", 1, 1, 1, 1),
	autonav(false)
{
	name = "Auto Nav System";
	abrv = "Nav";

	power_flags = POWER_WATTS | POWER_CRITICAL;
}

// +----------------------------------------------------------------------+

NavSystem::NavSystem(const NavSystem& s)
	: SimSystem(s),
	autonav(false)
{
	Mount(s);

	power_flags = POWER_WATTS | POWER_CRITICAL;
}

// +--------------------------------------------------------------------+

NavSystem::~NavSystem()
{
}

// +--------------------------------------------------------------------+

void NavSystem::ExecFrame(double seconds)
{
	// Run outside NavAI's steering callback: SetAutoLand replaces that director.
	if (rtb_recovery && autonav && ship && IsPowerOn()) {
		Instruction* Route = ship->GetNextNavPoint();
		if (!Route || Route->GetAction() != EInstruction::RTB || ship->IsDead() || ship->IsDying()) {
			rtb_recovery = false;
		}
		else {
			InboundSlot* Arrival = ship->GetInbound();
			if (Arrival && Arrival->Cleared() && ship->CanAutoLand()) {
				if (ship->SetAutoLand(true)) {
					ship->SetNavptStatus(Route, INSTRUCTION_STATUS::COMPLETE);
					UE_LOG(LogTemp, Display, TEXT("[AutoNavRTB] Recovery handed to autoland Ship='%hs'"), ship->GetName());
				}
			}
			else if ((recovery_call_delay -= FMath::Max(0.0, seconds)) <= 0.0) {
				Ship* Controller = Arrival && Arrival->GetDeck()
					? const_cast<Ship*>(Arrival->GetDeck()->GetCarrier()) : ship->GetController();
				if (Controller && !Controller->IsDead() && !Controller->IsDying()) {
					const RadioMessageAction Action = Arrival
						? RadioMessageAction::REQUEST_LANDING_CLEARANCE : RadioMessageAction::CALL_INBOUND;
					RadioTraffic::Transmit(new RadioMessage(Controller, ship, Action));
					UE_LOG(LogTemp, Display, TEXT("[AutoNavRTB] %s Ship='%hs' Controller='%hs'"),
						Arrival ? TEXT("Request clearance") : TEXT("Call inbound"), ship->GetName(), Controller->GetName());
				}
				recovery_call_delay = 10.0;
			}
		}
	}
	if (autonav && ship && !ship->GetNextNavPoint()) {
        UE_LOG(LogTemp, Display, TEXT("[AutoNavCancel] No next route instruction Ship='%hs'"), ship->GetName());
        autonav = false;
    }

	energy = 0.0f;
	SimSystem::ExecFrame(seconds);
}

// +----------------------------------------------------------------------+

bool NavSystem::AutoNavEngaged()
{
	return ship && autonav && IsPowerOn();
}

void NavSystem::EngageAutoNav()
{
	if (!IsPowerOn() || autonav)
		return;

	if (!ship) {
		UE_LOG(LogNavSystem, Warning, TEXT("EngageAutoNav called with null ship."));
		return;
	}

	if (!ship->GetNextNavPoint()) {
		UIButton::PlaySound(UIButton::SND_REJECT);
	}
	else {
		HUDSounds::PlaySound(HUDSounds::SND_NAV_MODE);
		autonav = true;
	}
}

void NavSystem::DisengageAutoNav()
{
	if (autonav)
		HUDSounds::PlaySound(HUDSounds::SND_NAV_MODE);

	autonav = false;
	rtb_recovery = false;
}

void NavSystem::BeginRTBRecovery()
{
	if (!autonav || rtb_recovery) return;
	rtb_recovery = true;
	recovery_call_delay = 0.0;
}

// +--------------------------------------------------------------------+

void NavSystem::Distribute(double delivered_energy, double seconds)
{
	if (!IsPowerOn())
		return;

	// Defensive: avoid divide-by-zero / negative step.
	if (seconds <= 0) {
		UE_LOG(LogNavSystem, Warning, TEXT("Distribute called with non-positive seconds (%.6f)."), seconds);
		return;
	}

	// convert Joules to Watts:
	energy = (float)(delivered_energy / seconds);

	// brown out:
	if (energy < capacity * 0.75f) {
        if (autonav) UE_LOG(LogTemp, Display, TEXT("[AutoNavCancel] Navigation power lost energy=%.3f capacity=%.3f"), energy, capacity);
        power_on = false;
	}

	// spike:
	else if (energy > capacity * 1.5f) {
        if (autonav) UE_LOG(LogTemp, Display, TEXT("[AutoNavCancel] Navigation power lost energy=%.3f capacity=%.3f"), energy, capacity);
        power_on = false;
		ApplyDamage(50);
	}
}
