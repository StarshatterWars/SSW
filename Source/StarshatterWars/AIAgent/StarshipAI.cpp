/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright(C) 2025 - 2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE : StarshipAI.cpp
    AUTHOR : Carlos Bott

    ORIGINAL AUTHOR AND STUDIO :
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997 - 2007. All Rights Reserved.

    OVERVIEW
    ========
    Starship(low - level) Artificial Intelligence class
*/

#include "StarshipAI.h"
#include "StarshipTacticalAI.h"

#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Mission.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "SimContact.h"
#include "WeaponGroup.h"
#include "Drive.h"
#include "Sim.h"
#include "StarSystem.h"
#include "FlightComputer.h"
#include "Farcaster.h"
#include "QuantumDrive.h"

#include "Game.h"
#include "Random.h"
#include "Solid.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "CoreMinimal.h" // UE_LOG
#include "Math/Vector.h" // FVector
#include "Math/UnrealMathUtility.h"

// +----------------------------------------------------------------------+

StarshipAI::StarshipAI(SimObject* s)
    : ShipAI(s),
    sub_select_time(0),
    point_defense_time(0),
    subtarget(0),
    tgt_point_defense(false)
{
    ai_type = ESteerAIType::STARSHIP;
    tactical = nullptr;

    // signifies this ship is a dead hulk:
    if (ship && ship->Design() && ship->Design()->auto_roll < 0) {
        FVector Torque(
            FMath::FRandRange(-16000.0f, 16000.0f),
            FMath::FRandRange(-16000.0f, 16000.0f),
            FMath::FRandRange(-16000.0f, 16000.0f)
        );

        Torque.Normalize();
        Torque *= float(ship->GetMass() / 10.0);

        ship->SetFLCSMode(EFLCSMode::MANUAL);

        if (ship->GetFLCS()) {
            ship->GetFLCS()->SetPowerOff();
        }

        ship->ApplyTorque(Torque);
        ship->SetVelocity(FMath::VRand() * FMath::FRandRange(20.0f, 50.0f));

        for (int i = 0; i < 64; i++) {
            Weapon* w = ship->GetWeaponByIndex(i + 1);

            if (w)
                w->DrainPower(0);
            else
                break;
        }
    }
    else {
        tactical = new StarshipTacticalAI(this);
    }

    sub_select_time = Game::GetGameTime() + FMath::RandRange(0, 2000);
    point_defense_time = sub_select_time;
}

// +--------------------------------------------------------------------+

StarshipAI::~StarshipAI()
{
}

// +--------------------------------------------------------------------+

void
StarshipAI::FindObjective()
{
    distance = 0.0;
    obj_w = FVector::ZeroVector;
    objective = FVector::ZeroVector;

    if (!ship)
    {
        return;
    }

    const RadioMessageAction order =
        ship->GetRadioOrders()
        ? ship->GetRadioOrders()->GetRadioAction()
        : RadioMessageAction::NONE;

    if (order == RadioMessageAction::QUANTUM_TO ||
        order == RadioMessageAction::FARCAST_TO)
    {
        FindObjectiveQuantum();
        objective = obj_w;
        return;
    }

    const bool bNoOrder =
        static_cast<int32>(order) == 0;

    const bool bHoldOrder =
        order == RadioMessageAction::WEP_HOLD ||
        order == RadioMessageAction::FORM_UP;

    const bool bForm =
        bHoldOrder ||
        (bNoOrder && !target) ||
        (farcaster != nullptr);

    Ship* ward =
        ship->GetWard();

    if (bForm && (element_index > 1 || ward))
    {
        ship->SetDirectorInfo("Formation");

        if (navpt &&
            navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH)
        {
            FindObjectiveNavPoint();
        }
        else
        {
            navpt = nullptr;
            FindObjectiveFormation();
        }

        objective = obj_w;
        return;
    }

    bool directed = false;
    double threat_level = 0.0;
    double support_level = 1.0;

    if (tactical)
    {
        directed =
            (tactical->RulesOfEngagement() == TacticalAI::DIRECTED);

        threat_level =
            tactical->ThreatLevel();

        support_level =
            tactical->SupportLevel();
    }

    if (bHoldOrder ||
        (!directed && threat_level >= 2.0 * support_level))
    {
        if (support)
        {
            const double d_support =
                (support->GetLocation() -
                    ship->GetLocation()).Size();

            if (d_support > 35e3)
            {
                ship->SetDirectorInfo("Regroup");
                FindObjectiveTarget(support);
                objective = obj_w;
                return;
            }
        }
        else if (threat && threat != target)
        {
            ship->SetDirectorInfo("Retreat");

            const FVector AwayFromThreat =
                ship->GetLocation() -
                threat->GetLocation();

            if (!AwayFromThreat.IsNearlyZero())
            {
                obj_w =
                    ship->GetLocation() +
                    AwayFromThreat.GetSafeNormal() * 100000.0f;
            }
            else
            {
                obj_w =
                    ship->GetLocation();
            }

            distance =
                (obj_w - ship->GetLocation()).Size();

            objective = obj_w;
            return;
        }
    }

    if (bHoldOrder)
    {
        if (navpt)
        {
            ship->SetDirectorInfo("Seek Navpoint");
            FindObjectiveNavPoint();
        }
        else if (patrol)
        {
            ship->SetDirectorInfo("Patrol");
            FindObjectivePatrol();
        }
        else
        {
            ship->SetDirectorInfo("Holding");
            obj_w = ship->GetLocation();
            distance = 0.0;
        }
    }
    else if (target)
    {
        ship->SetDirectorInfo("Seek Target");
        FindObjectiveTarget(target);
    }
    else if (patrol)
    {
        ship->SetDirectorInfo("Patrol");
        FindObjectivePatrol();
    }
    else if (ward)
    {
        ship->SetDirectorInfo("Seek Ward");
        FindObjectiveFormation();
    }
    else if (navpt)
    {
        ship->SetDirectorInfo("Seek Navpoint");
        FindObjectiveNavPoint();
    }
    else if (rumor)
    {
        ship->SetDirectorInfo("Search");
        FindObjectiveTarget(rumor);
    }
    else
    {
        obj_w = FVector::ZeroVector;
        objective = FVector::ZeroVector;
        distance = 0.0;
    }

    objective = obj_w;

    if (!objective.IsNearlyZero())
    {
        distance =
            (objective - ship->GetLocation()).Size();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[StarshipAI::FindObjective] Ship='%hs' Target='%hs' Ward='%hs' Navpt=%p Rumor='%hs' ObjectiveWorld=%s ObjW=%s ShipLoc=%s Distance=%.2f"),
        ship ? ship->GetName() : "NULL",
        target ? target->GetName() : "NULL",
        ward ? ward->GetName() : "NULL",
        navpt,
        rumor ? rumor->GetName() : "NULL",
        *objective.ToString(),
        *obj_w.ToString(),
        *ship->GetLocation().ToString(),
        distance);
}

// +--------------------------------------------------------------------+

void
StarshipAI::Navigator()
{
    ShipAI::Navigator();

    UE_LOG(LogTemp, Warning,
        TEXT("[StarshipAI::Navigator] Ship='%s' Target=%p Navpt=%p Throttle=%.2f Request=%.2f FLCSMode=%d"),
        ship ? ANSI_TO_TCHAR(ship->GetName()) : TEXT("NULL"),
        target,
        navpt,
        ship ? ship->GetThrottle() : -1.0,
        ship ? ship->GetThrottleRequest() : -1.0,
        ship ? static_cast<int32>(ship->GetFLCSMode()) : -1);
}
// +--------------------------------------------------------------------+

void
StarshipAI::HelmControl()
{
    if (!ship)
    {
        return;
    }

    if (ship->Design() && ship->Design()->auto_roll < 0)
    {
        return;
    }

    double trans_x = 0.0;
    double trans_y = 0.0;
    double trans_z = 0.0;

    const bool station_keeping =
        distance < 0.0;

    if (station_keeping)
    {
        accumulator.brake = 1.0;
        accumulator.stop = 1;

        ship->SetHelmPitch(0.0);
    }
    else
    {
        SimElement* elem =
            ship->GetElement();

        Ship* ward =
            ship->GetWard();

        Ship* s_threat =
            threat;

        if (other ||
            target ||
            ward ||
            s_threat ||
            navpt ||
            patrol ||
            farcaster ||
            element_index > 1)
        {
            ship->SetHelmHeading(accumulator.yaw);

            if (elem &&
                elem->Type() == static_cast<int32>(EMISSIONTYPE::FLIGHT_OPS))
            {
                ship->SetHelmPitch(0.0);

                if (ship->NumInbound() > 0)
                {
                    ship->SetHelmHeading(
                        ship->GetCompassHeading());
                }
            }
            else if (accumulator.pitch > 60.0 * DEGREES)
            {
                ship->SetHelmPitch(60.0 * DEGREES);
            }
            else if (accumulator.pitch < -60.0 * DEGREES)
            {
                ship->SetHelmPitch(-60.0 * DEGREES);
            }
            else
            {
                ship->SetHelmPitch(accumulator.pitch);
            }
        }
        else
        {
            ship->SetHelmPitch(0.0);
        }
    }

    ship->SetTransX(trans_x);
    ship->SetTransY(trans_y);
    ship->SetTransZ(trans_z);
}

void
StarshipAI::ThrottleControl()
{
    // signifies this ship is a dead hulk:
    if (!ship)
    {
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[StarshipAI::ThrottleControl]  Ship='%hs' This=%p"),
        ship ? ship->GetName() : "NULL",
        this);

    if (ship->Design() &&
        ship->Design()->auto_roll < 0)
    {
        return;
    }

    //-------------------------------------------------------------
    // Station keeping
    //-------------------------------------------------------------
    if (distance < 0.0)
    {
        old_throttle = 0.0;
        throttle = 0.0;

        ship->SetThrottle(0.0);
        ship->SetThrottleRequest(0.0);

        ship->SetTransX(0.0);
        ship->SetTransY(0.0);
        ship->SetTransZ(0.0);

        return;
    }

    const FVector ShipVel =
        ship->GetVelocity();

    const FVector ShipHeading =
        ship->GetHeading().GetSafeNormal();

    const double ship_speed =
        FVector::DotProduct(
            ShipVel,
            ShipHeading);

    double brakes = 0.0;

    Ship* ward =
        ship->GetWard();

    Ship* s_threat =
        threat;

    //-------------------------------------------------------------
    // Combat movement
    //-------------------------------------------------------------
    if (target || s_threat)
    {
        throttle = 100.0;

        if (target)
        {
            const FVector Delta =
                target->GetLocation() -
                ship->GetLocation();

            distance = Delta.Size();

            if (distance < 50e3)
            {
                const FVector DeltaDir =
                    Delta.GetSafeNormal();

                const double closing_speed =
                    FVector::DotProduct(
                        ShipVel,
                        DeltaDir);

                if (closing_speed > 300.0)
                {
                    throttle = 30.0;
                    brakes = 0.25;
                }
            }
        }

        throttle *= (1.0 - accumulator.brake);

        if (throttle < 1.0)
        {
            throttle = 0.0;
            brakes = 1.0;
        }
    }

    //-------------------------------------------------------------
    // Formation movement
    //-------------------------------------------------------------
    else if (ward)
    {
        const double lead_speed =
            ward->GetVelocity().Size();

        throttle = old_throttle;

        if (lead_speed > 0.0)
        {
            if (ship_speed > lead_speed)
            {
                throttle = old_throttle - 1.0;
                brakes = 0.2;
            }
            else if (ship_speed < lead_speed - 10.0)
            {
                throttle = old_throttle + 1.0;
            }
        }
        else
        {
            throttle = 0.0;
            brakes = 0.5;
        }
    }

    //-------------------------------------------------------------
    // Patrol / travel
    //-------------------------------------------------------------
    else if (patrol || farcaster)
    {
        throttle = 100.0;

        const double abs_ship_speed =
            FMath::Abs(ship_speed);

        if (distance < 10.0 * abs_ship_speed)
        {
            if (ShipVel.Size() > 200.0)
            {
                throttle = 5.0;
            }
            else
            {
                throttle = 50.0;
            }
        }
    }

    //-------------------------------------------------------------
    // Navpoint movement
    //-------------------------------------------------------------
    else if (navpt)
    {
        double speed =
            navpt->GetSpeed();

        throttle = old_throttle;

        if (hold)
        {
            throttle = 0.0;
            brakes = 1.0;
        }
        else
        {
            if (speed <= 0.0)
            {
                speed = 300.0;
            }

            if (ship_speed > speed)
            {
                if (throttle > 0.0 &&
                    old_throttle > 1.0)
                {
                    throttle =
                        old_throttle - 1.0;
                }

                brakes = 0.25;
            }
            else if (ship_speed < speed - 10.0)
            {
                throttle =
                    old_throttle + 1.0;
            }
        }
    }

    //-------------------------------------------------------------
    // Element following
    //-------------------------------------------------------------
    else if (element_index > 1)
    {
        Ship* lead =
            ship->GetElement()
            ? ship->GetElement()->GetShip(1)
            : nullptr;

        const double lead_speed =
            lead
            ? lead->GetVelocity().Size()
            : 0.0;

        const double delta_speed =
            lead_speed - ship_speed;

        const double delta_throttle =
            delta_speed * 1e-2 * seconds;

        throttle =
            old_throttle + delta_throttle;
    }

    //-------------------------------------------------------------
    // Idle
    //-------------------------------------------------------------
    else
    {
        throttle = 0.0;
    }

    //-------------------------------------------------------------
    // Debug fallback
    //-------------------------------------------------------------
#if 1
    if (throttle <= 0.0 &&
        !target &&
        !navpt &&
        !ward)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[StarshipAI::ThrottleControl] DEBUG FALLBACK Ship='%hs'"),
            ship ? ship->GetName() : "NULL");

        throttle = 100.0;
    }
#endif

    //-------------------------------------------------------------
    // Clamp
    //-------------------------------------------------------------
    throttle =
        FMath::Clamp(
            throttle,
            0.0,
            100.0);

    old_throttle =
        throttle;

    //-------------------------------------------------------------
    // Runtime throttle propagation
    //-------------------------------------------------------------
    ship->SetThrottle(throttle);
    ship->SetThrottleRequest(throttle);

    //-------------------------------------------------------------
    // IMPORTANT:
    // Disable legacy lateral translation until
    // UE local steering migration is complete.
    //-------------------------------------------------------------
    ship->SetTransX(0.0);
    ship->SetTransY(0.0);
    ship->SetTransZ(0.0);

    //-------------------------------------------------------------
    // Logging
    //-------------------------------------------------------------
    UE_LOG(LogTemp, Warning,
        TEXT("[StarshipAI::ThrottleControl] Ship='%hs' Target='%hs' Ward='%hs' Distance=%.2f ShipSpeed=%.2f Throttle=%.2f Request=%.2f Brakes=%.2f"),
        ship ? ship->GetName() : "NULL",
        target ? target->GetName() : "NULL",
        ward ? ward->GetName() : "NULL",
        distance,
        ship_speed,
        throttle,
        ship ? ship->GetThrottleRequest() : 0.0,
        brakes);
}

// +--------------------------------------------------------------------+

Steer
StarshipAI::SeekTarget()
{
    if (!ship)
    {
        return Steer();
    }

    if (navpt)
    {
        SimRegion* self_rgn =
            ship->GetRegion();

        SimRegion* nav_rgn =
            navpt->GetRegion();

        QuantumDrive* qdrive =
            ship->GetQuantumDrive();

        if (self_rgn && !nav_rgn)
        {
            nav_rgn =
                self_rgn;

            navpt->SetRegion(nav_rgn);
        }

        const bool use_farcaster =
            self_rgn &&
            nav_rgn &&
            self_rgn != nav_rgn &&
            (navpt->GetFarcast() ||
                !qdrive ||
                !qdrive->IsPowerOn() ||
                qdrive->GetStatus() < SYSTEM_STATUS::DEGRADED);

        if (use_farcaster)
        {
            if (!farcaster)
            {
                ListIter<Ship> s =
                    self_rgn->GetShips();

                while (++s && !farcaster)
                {
                    Ship* candidate =
                        s.value();

                    if (!candidate)
                    {
                        continue;
                    }

                    if (candidate->GetFarcaster())
                    {
                        const Ship* dest =
                            candidate->GetFarcaster()->GetDest();

                        if (dest &&
                            dest->GetRegion() == nav_rgn)
                        {
                            farcaster =
                                candidate->GetFarcaster();
                        }
                    }
                }
            }

            if (farcaster)
            {
                if (farcaster->GetShip() &&
                    farcaster->GetShip()->GetRegion() != self_rgn &&
                    farcaster->GetDest())
                {
                    farcaster =
                        farcaster->GetDest()->GetFarcaster();
                }

                obj_w =
                    farcaster->EndPoint();

                distance =
                    FVector(obj_w - ship->GetLocation()).Length();

                UE_LOG(LogTemp, Warning,
                    TEXT("[StarshipAI::SeekTarget] Ship='%hs' using farcaster Distance=%.2f ObjW=%s ShipLoc=%s"),
                    ship->GetName(),
                    distance,
                    *obj_w.ToString(),
                    *ship->GetLocation().ToString());

                //-------------------------------------------------
                // Objective arrival
                //-------------------------------------------------

                if (distance < 1000.0)
                {
                    bool bInCombat =
                        false;

                    Ship* TargetShip =
                        dynamic_cast<Ship*>(target);

                    if (TargetShip &&
                        TargetShip->GetIFF() != ship->GetIFF() &&
                        TargetShip->GetIFF() != 0)
                    {
                        bInCombat =
                            true;
                    }

                    if (threat &&
                        threat->GetIFF() != ship->GetIFF() &&
                        threat->GetIFF() != 0)
                    {
                        bInCombat =
                            true;
                    }

                    ship->SetNavptStatus(
                        navpt,
                        INSTRUCTION_STATUS::COMPLETE);

                    UE_LOG(LogTemp, Warning,
                        TEXT("[StarshipAI::SeekTarget] OBJECTIVE COMPLETE Ship='%hs' InCombat=%d Distance=%.2f"),
                        ship ? ship->GetName() : "NULL",
                        bInCombat ? 1 : 0,
                        distance);

                    if (!bInCombat)
                    {
                        farcaster =
                            nullptr;

                        throttle =
                            0.0;

                        old_throttle =
                            0.0;

                        ship->SetThrottle(0.0);
                        ship->SetThrottleRequest(0.0);

                        ship->SetTransX(0.0);
                        ship->SetTransY(0.0);
                        ship->SetTransZ(0.0);

                        Steer Stop;

                        Stop.brake =
                            1.0;

                        Stop.stop =
                            1;

                        return Stop;
                    }

                    farcaster =
                        nullptr;
                }
            }
        }
        else if (self_rgn &&
            nav_rgn &&
            self_rgn != nav_rgn)
        {
            QuantumDrive* q =
                ship->GetQuantumDrive();

            if (q &&
                q->ActiveState() == QuantumDrive::ACTIVE_READY)
            {
                q->SetDestination(
                    navpt->GetRegion(),
                    navpt->GetLocation());

                q->Engage();

                UE_LOG(LogTemp, Warning,
                    TEXT("[StarshipAI::SeekTarget] Ship='%hs' engaging quantum drive"),
                    ship->GetName());
            }
        }
    }

    const FVector BeforeObjective =
        objective;

    const FVector BeforeObjW =
        obj_w;

    Steer Result =
        ShipAI::SeekTarget();

    double ActualDistance =
        distance;

    if (target)
    {
        ActualDistance =
            (target->GetLocation() -
                ship->GetLocation()).Size();
    }
    else if (navpt &&
        navpt->GetTarget())
    {
        ActualDistance =
            (navpt->GetTarget()->GetLocation() -
                ship->GetLocation()).Size();
    }
    else if (navpt)
    {
        ActualDistance =
            (navpt->GetLocation() -
                ship->GetLocation()).Size();
    }
    else if (!obj_w.IsNearlyZero())
    {
        ActualDistance =
            (obj_w -
                ship->GetLocation()).Size();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[StarshipAI::SeekTarget] Ship='%hs' ShipLoc=%s Target='%hs' TargetLoc=%s Navpt=%p NavLoc=%s BeforeObj=%s AfterObj=%s BeforeObjW=%s AfterObjW=%s Distance=%.2f ResultYaw=%.4f ResultPitch=%.4f Brake=%.2f Stop=%d"),
        ship ? ship->GetName() : "NULL",
        ship ? *ship->GetLocation().ToString() : TEXT("NULL"),
        target ? target->GetName() : "NULL",
        target ? *target->GetLocation().ToString() : TEXT("NULL"),
        navpt,
        navpt ? *navpt->GetLocation().ToString() : TEXT("NULL"),
        *BeforeObjective.ToString(),
        *objective.ToString(),
        *BeforeObjW.ToString(),
        *obj_w.ToString(),
        ActualDistance,
        (double)Result.yaw,
        (double)Result.pitch,
        (double)Result.brake,
        (int)Result.stop);

    return Result;
}

// +--------------------------------------------------------------------+

Steer
StarshipAI::AvoidCollision()
{
    if (!ship || ship->GetVelocity().Length() < 25)
        return Steer();

    return ShipAI::AvoidCollision();
}

// +--------------------------------------------------------------------+

void
StarshipAI::FireControl()
{
    // identify unknown contacts:
    if (identify) {
        if (fabs(ship->GetHelmHeading() - ship->GetCompassHeading()) < 10 * DEGREES) {
            SimContact* contact = ship->FindContact(target);

            if (contact && !contact->ActLock()) {
                if (!ship->GetProbe()) {
                    ship->LaunchProbe();
                }
            }
        }

        return;
    }

    // investigate last known location of enemy ship:
    if (rumor && !target && ship->GetProbeLauncher() && !ship->GetProbe()) {
        // is rumor in basket?
        FVector Rmr = Transform(rumor->GetLocation());
        Rmr.Normalize();

        const double dx = fabs(Rmr.X);
        const double dy = fabs(Rmr.Y);

        if (dx < 10 * DEGREES && dy < 10 * DEGREES && Rmr.Z > 0) {
            ship->LaunchProbe();
        }
    }

    // Corvettes and Frigates are anti-air platforms.  They need to
    // target missile threats even when the threat is aimed at another
    // friendly ship.  Forward facing weapons must be on auto fire,
    // while lateral and aft facing weapons are set to point defense.
    if (ship->Class() == CLASSIFICATION::CORVETTE || ship->Class() == CLASSIFICATION::FRIGATE)
    {
        ListIter<WeaponGroup> grp_iter = ship->GetWeapons();
        while (++grp_iter)
        {
            WeaponGroup* group = grp_iter.value();

            ListIter<Weapon> w_iter = group->GetWeapons();
            while (++w_iter)
            {
                Weapon* weapon = w_iter.value();

                const double weapon_az = weapon->GetAzimuth();

                if (fabs(weapon_az) < 45 * DEGREES)
                {
                    weapon->SetFiringOrders(WeaponsOrders::AUTO);
                    weapon->SetTarget(target, nullptr);
                }
                else
                {
                    weapon->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
                }
            }
        }
    }

    // All other starships are free to engage ship targets.  Weapon
    // fire control is managed by the type of weapon.
    else {
        SimSystem* subtgt = SelectSubtarget();

        ListIter<WeaponGroup> grp_iter = ship->GetWeapons();
        while (++grp_iter) {
            WeaponGroup* group = grp_iter.value();

            if (group->GetDesign()->target_type & (int)CLASSIFICATION::DROPSHIPS) { // anti-air weapon?
                group->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
            }
            else if (group->IsDrone()) { // torpedoes
                group->SetFiringOrders(WeaponsOrders::MANUAL);
                group->SetTarget(target, 0);

                if (target && target->GetRegion() == ship->GetRegion()) {
                    const FVector Delta = target->GetLocation() - ship->GetLocation();
                    const double  range = Delta.Length();

                    if (range < group->GetDesign()->max_range * 0.9 &&
                        !AssessTargetPointDefense()) {
                        group->SetFiringOrders(WeaponsOrders::AUTO);
                    }
                    else if (range < group->GetDesign()->max_range * 0.5) {
                        group->SetFiringOrders(WeaponsOrders::AUTO);
                    }
                }
            }
            else { // anti-ship weapon
                group->SetFiringOrders(WeaponsOrders::AUTO);
                group->SetTarget(target, subtgt);
                group->SetSweep(subtgt ? WeaponsSweep::SWEEP_NONE : WeaponsSweep::SWEEP_TIGHT);
            }
        }
    }
}

// +--------------------------------------------------------------------+

SimSystem*
StarshipAI::SelectSubtarget()
{
    // NOTE: Return type is SimSystem* per your instruction.
    // This function currently returns a Weapon* subtarget, so we cast at the return boundary
    // to keep existing call sites intact while you finish the broader type migration.

    const uint32 NowMs = Game::GetGameTime();

    if ((NowMs - sub_select_time) < 2345u)
        return (SimSystem*)subtarget;

    subtarget = nullptr;

    if (!target || target->GetType() != SimObject::SIM_SHIP || GetAILevel() < 1)
        return (SimSystem*)subtarget;

    Ship* tgt_ship = (Ship*)target;

    if (!tgt_ship->IsStarship())
        return (SimSystem*)subtarget;

    Weapon* subtgt = nullptr;
    double  dist = 50e3;

    // Vector from target -> ship (same directionality as original)
    const FVector Svec = ship->GetLocation() - tgt_ship->GetLocation();

    sub_select_time = NowMs;

    // first pass: turrets
    ListIter<WeaponGroup> g_iter = tgt_ship->GetWeapons();
    while (++g_iter) {
        WeaponGroup* g = g_iter.value();

        if (g->GetDesign() && g->GetDesign()->turret_model) {
            ListIter<Weapon> w_iter = g->GetWeapons();
            while (++w_iter) {
                Weapon* w = w_iter.value();

                if (!w || w->GetAvailability() < 35)
                    continue;

                // UE fix: dot product
                if (FVector::DotProduct(w->GetAimVector(), Svec) < 0.0f)
                    continue;

                if (w->GetTurret()) {
                    // C2737 FIX:
                    // Your build is treating this as a "const object must be initialized" case.
                    // Avoid declaring a const local here; initialize a non-const temp instead.
                    FVector Tloc;
                    Tloc = w->GetTurret()->Location();

                    const FVector Delta = Tloc - ship->GetLocation();
                    const double  Dlen = (double)Delta.Length();

                    if (Dlen < dist) {
                        subtgt = w;
                        dist = Dlen;
                    }
                }
            }
        }
    }

    // second pass: major weapons
    if (!subtgt) {
        g_iter.reset();
        while (++g_iter) {
            WeaponGroup* g = g_iter.value();

            if (g->GetDesign() && !g->GetDesign()->turret_model) {
                ListIter<Weapon> w_iter = g->GetWeapons();
                while (++w_iter) {
                    Weapon* w = w_iter.value();

                    if (!w || w->GetAvailability() < 35)
                        continue;

                    // UE fix: dot product
                    if (FVector::DotProduct(w->GetAimVector(), Svec) < 0.0f)
                        continue;

                    // FIX (C2737): ensure Tloc is initialized even if MountLocation() is not const-correct
                    // or is being treated like an lvalue on your toolchain.
                    const FVector Tloc = w->GetMountLocation();
                    const FVector Delta = Tloc - ship->GetLocation();
                    const double  Dlen = (double)Delta.Length();

                    if (Dlen < dist) {
                        subtgt = w;
                        dist = Dlen;
                    }
                }
            }
        }
    }

    subtarget = subtgt;
    return (SimSystem*)subtarget;
}


// +--------------------------------------------------------------------+

bool
StarshipAI::AssessTargetPointDefense()
{
    if (Game::GetGameTime() - point_defense_time < 3500)
        return tgt_point_defense;

    tgt_point_defense = false;

    if (!target || target->GetType() != SimObject::SIM_SHIP || GetAILevel() < 2)
        return tgt_point_defense;

    Ship* tgt_ship = (Ship*)target;

    if (!tgt_ship->IsStarship())
        return tgt_point_defense;

    FVector Svec = ship->GetLocation() - tgt_ship->GetLocation();

    point_defense_time = Game::GetGameTime();

    // first pass: turrets
    ListIter<WeaponGroup> g_iter = tgt_ship->GetWeapons();
    while (++g_iter && !tgt_point_defense) {
        WeaponGroup* g = g_iter.value();

        if (g->CanTarget(1)) {
            ListIter<Weapon> w_iter = g->GetWeapons();
            while (++w_iter && !tgt_point_defense) {
                Weapon* w = w_iter.value();

                if (w->GetAvailability() > 35 &&
                    FVector::DotProduct(w->GetAimVector(), Svec) > 0.0)
                {
                    tgt_point_defense = true;
                }
            }
        }
    }

    return tgt_point_defense;
}

// +--------------------------------------------------------------------+

FVector
StarshipAI::Transform(const FVector& Point)
{
    if (!ship)
    {
        return FVector::ZeroVector;
    }

    //-------------------------------------------------------------
    // Convert WORLD target point into LOCAL steering space:
    //
    // X = right
    // Y = up
    // Z = forward
    //
    // VERIFIED SENSOR BASIS:
    // VPN = Forward
    // VUP = Up
    // VRT = Right
    //-------------------------------------------------------------

    const FVector WorldDir =
        Point - ship->GetLocation();

    const FVector Forward =
        ship->GetCam().vpn().GetSafeNormal();

    const FVector Up =
        ship->GetCam().vup().GetSafeNormal();

    const FVector Right =
        ship->GetCam().vrt().GetSafeNormal();

    const FVector Local(
        FVector::DotProduct(WorldDir, Right),
        FVector::DotProduct(WorldDir, Up),
        FVector::DotProduct(WorldDir, Forward));

    return Local;
}


Steer
StarshipAI::Seek(const FVector& Point)
{
    Steer Result;

    //-------------------------------------------------------------
    // LOCAL steering space:
    //
    // X = right
    // Y = up
    // Z = forward
    //-------------------------------------------------------------

    const double Right =
        Point.X;

    const double Up =
        Point.Y;

    const double Forward =
        Point.Z;

    Result.yaw =
        atan2(Right, Forward);

    const double FlatDist =
        sqrt((Right * Right) + (Forward * Forward));

    if (FlatDist > KINDA_SMALL_NUMBER)
    {
        Result.pitch =
            -atan2(Up, FlatDist);
    }
    else
    {
        Result.pitch = 0.0;
    }

#if PLATFORM_WINDOWS
    if (!_finite(Result.yaw))
    {
        Result.yaw = 0.0;
    }

    if (!_finite(Result.pitch))
    {
        Result.pitch = 0.0;
    }
#else
    if (!isfinite(Result.yaw))
    {
        Result.yaw = 0.0;
    }

    if (!isfinite(Result.pitch))
    {
        Result.pitch = 0.0;
    }
#endif

    return Result;
}

Steer
StarshipAI::Flee(const FVector& Point)
{
    Steer Result =
        Seek(Point);

    Result.yaw += PI;

    return Result;
}

Steer
StarshipAI::Avoid(const FVector& Point, float Radius)
{
    Steer Result = Seek(Point);

    if (Point.X > 0.0)
    {
        Result.yaw -= PI / 2.0;
    }
    else
    {
        Result.yaw += PI / 2.0;
    }

    (void)Radius;

    return Result;
}
