/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         GroundAI.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Low-Level Artificial Intelligence class for Ground Units
*/

#include "GroundAI.h"

#include "SteerAI.h"
#include "SimSystem.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Shield.h"
#include "Sim.h"
#include "PlayerCharacter.h"
#include "CarrierAI.h"
#include "SimContact.h"
#include "Weapon.h"
#include "WeaponGroup.h"

#include "Game.h"
#include "Physical.h"
#include "GameStructs.h"

#include "CoreMinimal.h" // UE_LOG

// NOTE:
// - Removed MemDebug.h (not supported in UE builds).
// - Replaced sprintf_s debug naming with UE-safe snprintf variants.
// - Kept core Starshatter types (ListIter, etc.) intact.

// +----------------------------------------------------------------------+

GroundAI::GroundAI(SimObject* s)
    : ship((Ship*)s),
    target(0),
    subtarget(0),
    exec_time(0.0),
    carrier_ai(0)
{
    Sim* sim = Sim::GetSim();
    Ship* pship = sim ? sim->GetPlayerShip() : nullptr;
    int   player_team = 1;
    int   ai_level = 1;

    if (pship)
        player_team = pship->GetIFF();

    PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
    if (player) {
        if (ship && ship->GetIFF() && ship->GetIFF() != player_team) {
            ai_level = player->AILevel();
        }
        else if (player->AILevel() == 0) {
            ai_level = 1;
        }
    }

    // evil alien ships are *always* smart:
    if (ship && ship->GetIFF() > 1 && ship->Design()->auto_roll > 1) {
        ai_level = 2;
    }

    if (ship && ship->GetHangar() && ship->GetCommandAILevel() > 0)
        carrier_ai = new  CarrierAI(ship, ai_level);
}

// +--------------------------------------------------------------------+

GroundAI::~GroundAI()
{
    delete carrier_ai;
}

// +--------------------------------------------------------------------+

void
GroundAI::SetTarget(SimObject* targ, SimSystem* sub)
{
    if (target != targ) {
        target = targ;

        if (target)
            Observe(target);
    }

    subtarget = sub;
}

// +--------------------------------------------------------------------+

bool
GroundAI::Update(SimObject* obj)
{
    if (obj == target) {
        target = 0;
        subtarget = 0;
    }

    return SimObserver::Update(obj);
}

const char*
GroundAI::GetObserverName() const
{
    static thread_local char NameBuf[64];

#if PLATFORM_WINDOWS
    _snprintf_s(NameBuf, sizeof(NameBuf), _TRUNCATE, "GroundAI(%s)", ship ? ship->Name() : "null");
#else
    snprintf(NameBuf, sizeof(NameBuf), "GroundAI(%s)", ship ? ship->Name() : "null");
#endif

    return NameBuf;
}

// +--------------------------------------------------------------------+

void
GroundAI::SelectTarget()
{
    SimObject* potential_target = 0;

    // pick the closest combatant ship with a different IFF code:
    double target_dist = 1.0e15;

    Ship* current_ship_target = 0;

    ListIter<SimContact> c_iter = ship->ContactList();
    while (++c_iter) {
        SimContact* contact = c_iter.value();
        int      c_iff = contact->GetIFF(ship);
        Ship* c_ship = contact->GetShip();
        SimShot* c_shot = contact->GetShot();
        bool     rogue = false;

        (void)c_shot;

        if (c_ship)
            rogue = c_ship->IsRogue();

        if (rogue || (c_iff > 0 && c_iff != ship->GetIFF() && c_iff < 1000)) {
            if (c_ship && !c_ship->InTransition()) {
                // found an enemy, check distance:
                const double dist = (ship->Location() - c_ship->Location()).Length();

                if (!current_ship_target ||
                    (c_ship->Class() <= current_ship_target->Class() && dist < target_dist)) {
                    current_ship_target = c_ship;
                    target_dist = dist;
                }
            }
        }

        potential_target = current_ship_target;
    }

    SetTarget(potential_target);
}

// +--------------------------------------------------------------------+

int
GroundAI::Type() const
{
    return SteerAI::GROUND;
}

// +--------------------------------------------------------------------+

void
GroundAI::ExecFrame(double secs)
{
    const double exec_period_ms = 1000.0;

    // Game::GameTime() is assumed to be milliseconds (legacy Starshatter behavior).
    if ((double)Game::GameTime() - exec_time > exec_period_ms) {
        exec_time = (double)Game::GameTime();
        SelectTarget();
    }

    if (ship) {
        Shield* shield = ship->GetShield();

        if (shield)
            shield->SetPowerLevel(100);

        ListIter<WeaponGroup> iter = ship->GetWeapons();
        while (++iter) {
            WeaponGroup* group = (WeaponGroup*)iter.value();

            if (group->NumWeapons() > 1 && group->CanTarget((int)CLASSIFICATION::DROPSHIPS))
                group->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
            else
                group->SetFiringOrders(WeaponsOrders::AUTO);

            group->SetTarget((Ship*)target, 0);
        }

        if (carrier_ai)
            carrier_ai->ExecFrame(secs);
    }
}
