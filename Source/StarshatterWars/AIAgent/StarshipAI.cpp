/*  Project Starshatter Wars
    Fractal Dev Studios

    StarshipAI.cpp

    UE port version restored from legacy Stars45 StarshipAI.cpp.

    Rule:
    Legacy sim owns movement, steering, targeting, FLCS, physics.
    Unreal is visualization only.
*/

#include "StarshipAI.h"
#include "StarshipTacticalAI.h"

#include "Ship.h"
#include "Solid.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "Mission.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "SimContact.h"
#include "WeaponGroup.h"
#include "Weapon.h"
#include "Drive.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "FlightComputer.h"
#include "Farcaster.h"
#include "QuantumDrive.h"

#include "Game.h"
#include "Random.h"

#include "CoreMinimal.h"
#include "ObjectiveArrivalUtils.h"

// +----------------------------------------------------------------------+

StarshipAI::StarshipAI(SimObject* s)
    : ShipAI(s),
    sub_select_time(0),
    point_defense_time(0),
    subtarget(nullptr),
    tgt_point_defense(false)
{
    ai_type =
        ESteerAIType::STARSHIP;

    //-------------------------------------------------------------
    // Legacy dead hulk behavior
    //-------------------------------------------------------------
    if (ship &&
        ship->Design() &&
        ship->Design()->auto_roll < 0)
    {
        FVector Torque(
            FMath::FRandRange(-16000.0f, 16000.0f),
            FMath::FRandRange(-16000.0f, 16000.0f),
            FMath::FRandRange(-16000.0f, 16000.0f));

        Torque.Normalize();
        Torque *=
            float(ship->GetMass() / 10.0);

        ship->SetFLCSMode(
            EFLCSMode::MANUAL);

        if (ship->GetFLCS())
        {
            ship->GetFLCS()->SetPowerOff();
        }

        ship->ApplyTorque(Torque);

        ship->SetVelocity(
            FMath::VRand() *
            FMath::FRandRange(20.0f, 50.0f));

        for (int i = 0; i < 64; i++)
        {
            Weapon* W =
                ship->GetWeaponByIndex(i + 1);

            if (W)
            {
                W->DrainPower(0);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        tactical =
            new StarshipTacticalAI(this);
    }

    sub_select_time =
        Game::GetGameTime() +
        (DWORD)FMath::RandRange(0, 2000);

    point_defense_time =
        sub_select_time;
}

// +----------------------------------------------------------------------+

StarshipAI::~StarshipAI()
{
}

// +----------------------------------------------------------------------+

void
StarshipAI::FindObjective()
{
    distance =
        0.0;

    obj_w =
        FVector::ZeroVector;

    objective =
        FVector::ZeroVector;

    if (!ship)
    {
        return;
    }

    RadioMessageAction Order =
        RadioMessageAction::NONE;

    if (ship->GetRadioOrders())
    {
        Order =
            ship->GetRadioOrders()->GetRadioAction();
    }

    //-------------------------------------------------------------
    // Quantum / Farcast
    //-------------------------------------------------------------
    if (Order == RadioMessageAction::QUANTUM_TO ||
        Order == RadioMessageAction::FARCAST_TO)
    {
        FindObjectiveQuantum();

        objective =
            Transform(obj_w);

        distance =
            objective.Size();

        return;
    }

    const bool bHold =
        Order == RadioMessageAction::WEP_HOLD ||
        Order == RadioMessageAction::FORM_UP;

    const bool bNoOrder =
        static_cast<int32>(Order) == 0;

    const bool bForm =
        bHold ||
        (bNoOrder && !target) ||
        farcaster != nullptr;

    //-------------------------------------------------------------
    // If not element leader, stay in formation
    //-------------------------------------------------------------
    if (bForm &&
        element_index > 1)
    {
        ship->SetDirectorInfo("Formation");

        if (navpt &&
            navpt->GetAction() ==
            EInstruction::Launch)
        {
            FindObjectiveNavPoint();
        }
        else
        {
            navpt =
                nullptr;

            FindObjectiveFormation();
        }

        objective =
            Transform(obj_w);

        distance =
            objective.Size();

        return;
    }

    //-------------------------------------------------------------
    // Tactical state
    //-------------------------------------------------------------
    bool bDirected =
        false;

    double ThreatLevel =
        0.0;

    double SupportLevel =
        1.0;

    Ship* Ward =
        ship->GetWard();

    if (tactical)
    {
        bDirected =
            tactical->RulesOfEngagement() ==
            TacticalAI::DIRECTED;

        ThreatLevel =
            tactical->ThreatLevel();

        SupportLevel =
            tactical->SupportLevel();
    }

    //-------------------------------------------------------------
    // Threat processing
    //-------------------------------------------------------------
    if (bHold ||
        (!bDirected &&
            ThreatLevel >= 2.0 * SupportLevel))
    {
        //---------------------------------------------------------
        // Seek support
        //---------------------------------------------------------
        if (support)
        {
            const double SupportDistance =
                (support->GetLocation() -
                    ship->GetLocation()).Size();

            if (SupportDistance > 35e3)
            {
                ship->SetDirectorInfo("Regroup");

                FindObjectiveTarget(support);

                objective =
                    Transform(obj_w);

                distance =
                    objective.Size();

                return;
            }
        }

        //---------------------------------------------------------
        // Run away
        //---------------------------------------------------------
        else if (threat &&
            threat != target)
        {
            ship->SetDirectorInfo("Retreat");

            obj_w =
                ship->GetLocation() +
                (ship->GetLocation() -
                    threat->GetLocation()) * 100.0f;

            objective =
                Transform(obj_w);

            distance =
                objective.Size();

            return;
        }
    }

    //-------------------------------------------------------------
    // Weapons hold
    //-------------------------------------------------------------
    if (bHold)
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

            obj_w =
                ship->GetLocation();

            objective =
                FVector::ZeroVector;

            distance =
                0.0;

            return;
        }
    }

    //-------------------------------------------------------------
    // Normal processing
    //-------------------------------------------------------------
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
    else if (Ward)
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
        obj_w =
            ship->GetLocation();

        objective =
            FVector::ZeroVector;

        distance =
            0.0;

        return;
    }

    //-------------------------------------------------------------
    // Legacy StarshipAI rule:
    // obj_w is world-space.
    // objective is relative-world-space.
    //-------------------------------------------------------------
    objective =
        Transform(obj_w);

    distance =
        objective.Size();

    if (!_stricmp(ship->GetName(), "Blockade Runner"))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[StarshipAI::FindObjective BR] ")
            TEXT("ObjW=%s ObjectiveRelative=%s ShipLoc=%s Distance=%.2f"),
            *obj_w.ToString(),
            *objective.ToString(),
            *ship->GetLocation().ToString(),
            distance);
    }
}

// +----------------------------------------------------------------------+

void
StarshipAI::Navigator()
{
    //-------------------------------------------------------------
    // Dead hulk
    //-------------------------------------------------------------
    if (ship &&
        ship->Design() &&
        ship->Design()->auto_roll < 0)
    {
        ship->SetDirectorInfo("Dead");
        return;
    }

    accumulator.Clear();

    magnitude =
        0.0;

    hold =
        false;

    if ((ship->GetElement() &&
        ship->GetElement()->GetHoldTime() > 0) ||
        (navpt &&
            navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE &&
            navpt->GetHoldTime() > 0))
    {
        hold =
            true;
    }

    ship->SetFLCSMode(
        EFLCSMode::HELM);

    if (!ship->GetDirectorInfo())
    {
        if (target)
        {
            ship->SetDirectorInfo("Seek Target");
        }
        else if (ship->GetWard())
        {
            ship->SetDirectorInfo("Seek Ward");
        }
        else
        {
            ship->SetDirectorInfo("Patrol");
        }
    }

    //-------------------------------------------------------------
    // Legacy steering selection
    //-------------------------------------------------------------
    if (farcaster &&
        distance < 25e3)
    {
        accumulator =
            SeekTarget();
    }
    else
    {
        accumulator =
            AvoidCollision();

        if (!other &&
            !hold)
        {
            accumulator =
                SeekTarget();
        }
    }

    HelmControl();
    ThrottleControl();
    FireControl();
    AdjustDefenses();
}

// +----------------------------------------------------------------------+

void
StarshipAI::HelmControl()
{
    if (!ship)
    {
        return;
    }

    //-------------------------------------------------------------
    // Dead hulk
    //-------------------------------------------------------------
    if (ship->Design() &&
        ship->Design()->auto_roll < 0)
    {
        return;
    }

    double trans_x = 0.0;
    double trans_y = 0.0;
    double trans_z = 0.0;

    const bool station_keeping =
        distance < 0.0;

    //-------------------------------------------------------------
    // Station keeping
    //-------------------------------------------------------------
    if (station_keeping)
    {
        accumulator.brake = 1.0;
        accumulator.stop = 1;

        ship->SetHelmPitch(0.0);
    }
    else
    {
        SimElement* Elem =
            ship->GetElement();

        Ship* Ward =
            ship->GetWard();

        Ship* StrongThreat =
            nullptr;

        if (threat &&
            threat->GetClassification() >= ship->GetClassification())
        {
            StrongThreat = threat;
        }

        if (other ||
            target ||
            Ward ||
            StrongThreat ||
            navpt ||
            patrol ||
            farcaster ||
            element_index > 1)
        {
            //-----------------------------------------------------
            // Legacy:
            // accumulator.yaw is ABSOLUTE desired helm heading.
            // Do not call LookAt() here.
            // Do not directly rewrite cam/vpn here.
            //-----------------------------------------------------
            ship->SetHelmHeading(
                accumulator.yaw);

            if (Elem &&
                Elem->GetMissionType() ==
                static_cast<int32>(EMissionType::FLIGHT_OPS))
            {
                ship->SetHelmPitch(0.0);

                if (ship->NumInbound() > 0)
                {
                    ship->SetHelmHeading(
                        ship->GetCompassHeading());
                }
            }
            else if (accumulator.pitch >
                60.0 * DEGREES)
            {
                ship->SetHelmPitch(
                    60.0 * DEGREES);
            }
            else if (accumulator.pitch <
                -60.0 * DEGREES)
            {
                ship->SetHelmPitch(
                    -60.0 * DEGREES);
            }
            else
            {
                ship->SetHelmPitch(
                    accumulator.pitch);
            }

            if (ship &&
                !_stricmp(ship->GetName(), "Blockade Runner"))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[StarshipAI::HelmControl BR] ")
                    TEXT("AccumYaw=%.6f AccumPitch=%.6f Compass=%.6f VPN=%s ThrottleReq=%.2f"),
                    accumulator.yaw,
                    accumulator.pitch,
                    ship->GetCompassHeading(),
                    *ship->GetCam().vpn().ToString(),
                    ship->GetThrottleRequest());
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

    //-------------------------------------------------------------
    // Keep FLCS single-call only.
    //-------------------------------------------------------------
    // ship->ExecFLCSFrame();
}

// +----------------------------------------------------------------------+
void
StarshipAI::ThrottleControl()
{
    if (!ship)
    {
        return;
    }

    if (ship->Design() &&
        ship->Design()->auto_roll < 0)
    {
        return;
    }

    if (distance < 0.0)
    {
        old_throttle =
            0.0;

        throttle =
            0.0;

        ship->SetThrottle(
            0.0);

        ship->SetThrottleRequest(
            0.0);

        return;
    }

    const FVector ShipVelocity =
        ship->GetVelocity();

    const FVector ShipHeading =
        ship->GetHeading().GetSafeNormal();

    const double ship_speed =
        FVector::DotProduct(
            ShipVelocity,
            ShipHeading);

    double brakes =
        0.0;

    Ship* Ward =
        ship->GetWard();

    Ship* StrongThreat =
        nullptr;

    if (threat &&
        threat->GetClassification() >=
        ship->GetClassification())
    {
        StrongThreat =
            threat;
    }

    //-------------------------------------------------------------
    // Combat / target pursuit
    //-------------------------------------------------------------

    if (target ||
        StrongThreat)
    {
        throttle =
            100.0;

        if (target &&
            distance < 50e3)
        {
            double closing_speed =
                ship_speed;

            FVector Delta =
                target->GetLocation() -
                ship->GetLocation();

            if (!Delta.IsNearlyZero())
            {
                Delta.Normalize();

                closing_speed =
                    FVector::DotProduct(
                        ship->GetVelocity(),
                        Delta);
            }

            if (closing_speed > 300.0)
            {
                throttle =
                    30.0;

                brakes =
                    0.25;
            }
        }

        throttle *=
            (1.0 - accumulator.brake);
    }

    //-------------------------------------------------------------
    // Formation / ward following
    //-------------------------------------------------------------

    else if (Ward)
    {
        double speed =
            Ward->GetVelocity().Size();

        throttle =
            old_throttle;

        if (speed == 0.0)
        {
            const double d =
                (ship->GetLocation() -
                    Ward->GetLocation()).Size();

            if (d > 30e3)
            {
                speed =
                    (d - 30e3) / 100.0;
            }
        }

        if (speed > 0.0)
        {
            if (ship_speed > speed)
            {
                throttle =
                    old_throttle - 1.0;

                brakes =
                    0.2;
            }
            else if (ship_speed < speed - 10.0)
            {
                throttle =
                    old_throttle + 1.0;
            }
        }
        else
        {
            throttle =
                0.0;

            brakes =
                0.5;
        }
    }

    //-------------------------------------------------------------
    // Patrol / farcaster transit
    //-------------------------------------------------------------

    else if (patrol ||
        farcaster)
    {
        throttle =
            100.0;
    }

    //-------------------------------------------------------------
    // Navpoint speed control
    //-------------------------------------------------------------

    else if (navpt)
    {
        double speed =
            navpt->GetSpeed();

        throttle =
            old_throttle;

        if (hold)
        {
            throttle =
                0.0;

            brakes =
                1.0;
        }
        else
        {
            if (speed <= 0.0)
            {
                speed =
                    300.0;
            }

            if (ship_speed > speed)
            {
                if (throttle > 0.0 &&
                    old_throttle > 1.0)
                {
                    throttle =
                        old_throttle - 1.0;
                }

                brakes =
                    0.25;
            }
            else if (ship_speed < speed - 10.0)
            {
                throttle =
                    old_throttle + 1.0;
            }
        }
    }

    //-------------------------------------------------------------
    // Formation element following
    //-------------------------------------------------------------

    else if (element_index > 1)
    {
        Ship* Lead =
            ship->GetElement()
            ? ship->GetElement()->GetShip(1)
            : nullptr;

        if (Lead)
        {
            const double lv =
                Lead->GetVelocity().Size();

            const double sv =
                ship_speed;

            const double dv =
                lv - sv;

            throttle =
                old_throttle +
                dv * 1e-2 * seconds;
        }
        else
        {
            throttle =
                0.0;
        }
    }

    //-------------------------------------------------------------
    // Idle
    //-------------------------------------------------------------

    else
    {
        throttle =
            0.0;
    }

    //-------------------------------------------------------------
    // Objective arrival braking
    //-------------------------------------------------------------

    if (navpt ||
        farcaster ||
        patrol)
    {
        const EObjectiveArrivalType ArrivalType =
            DetermineArrivalType();

        const FObjectiveArrivalSettings ArrivalSettings =
            FObjectiveArrivalUtils::MakeSettings(
                ArrivalType,
                ship);

        const FObjectiveArrivalState ArrivalState =
            FObjectiveArrivalUtils::EvaluateArrival(
                ship->GetLocation(),
                ship->GetVelocity(),
                obj_w,
                ArrivalSettings);

        ApplyObjectiveArrivalBraking(
            ArrivalSettings,
            ArrivalState,
            throttle,
            brakes);

        //---------------------------------------------------------
        // React only.
        // ShipAI owns bObjectiveArrived.
        //---------------------------------------------------------

        if (ArrivalState.bComplete)
        {
            throttle =
                0.0;

            brakes =
                1.0;
        }
    }

    //-------------------------------------------------------------
    // Final clamp
    //-------------------------------------------------------------

    throttle =
        FMath::Clamp(
            throttle,
            0.0,
            100.0);

    old_throttle =
        throttle;

    ship->SetThrottle(
        throttle);

    ship->SetThrottleRequest(
        throttle);

    //-------------------------------------------------------------
    // FLCS braking
    //-------------------------------------------------------------

    if (ship_speed > 1.0 &&
        brakes > 0.0 &&
        ship->Design())
    {
        ship->SetTransY(
            -brakes *
            ship->Design()->trans_y);
    }
    else
    {
        ship->SetTransY(
            0.0);
    }

    //-------------------------------------------------------------
    // Debug
    //-------------------------------------------------------------

    const double TargetDistance =
        target
        ? (target->GetLocation() -
            ship->GetLocation()).Size()
        : -1.0;

    DebugThrottleControl(
        "FINAL",
        ship_speed,
        throttle,
        brakes,
        distance,
        TargetDistance,
        -1.0,
        -1.0);
}

void
StarshipAI::ApplyObjectiveArrivalBraking(
    const FObjectiveArrivalSettings& ArrivalSettings,
    const FObjectiveArrivalState& ArrivalState,
    double& InOutThrottle,
    double& InOutBrakes)
{
    if (!ship)
    {
        return;
    }

    const double Distance =
        ArrivalState.Distance;

    const double BrakeRadius =
        ArrivalSettings.BrakeRadius;

    if (BrakeRadius <= 0.0)
    {
        return;
    }

    if (Distance < BrakeRadius)
    {
        const double BrakeAlpha =
            FMath::Clamp(
                1.0 - (Distance / ArrivalSettings.BrakeRadius),
                0.0,
                1.0);

        const double EasedBrakeAlpha =
            1.0 -
            FMath::Pow(
                1.0 - BrakeAlpha,
                5.0);

        const double ArrivalBrake =
            FMath::Lerp(
                0.55,
                1.0,
                EasedBrakeAlpha);

        InOutBrakes =
            FMath::Max(
                InOutBrakes,
                ArrivalBrake);

        if (EasedBrakeAlpha > 0.85)
        {
            InOutThrottle =
                0.0;
        }
        else
        {
            const double ThrottleScale =
                FMath::Lerp(
                    1.0,
                    0.15,
                    EasedBrakeAlpha);

            InOutThrottle *=
                ThrottleScale;
        }

        const double TargetDistance =
            target
            ? (target->GetLocation() -
                ship->GetLocation()).Size()
            : -1.0;

        DebugThrottleControl(
            "ARRIVAL_BRAKE",
            ship->GetVelocity().Size(),
            InOutThrottle,
            InOutBrakes,
            Distance,
            TargetDistance,
            BrakeAlpha,
            EasedBrakeAlpha);
    }

    if (Distance <= ArrivalSettings.ArrivalRadius)
    {
        InOutThrottle =
            0.0;

        InOutBrakes =
            1.0;
    }
}
// +----------------------------------------------------------------------+

Steer
StarshipAI::SeekTarget()
{
    if (!ship)
    {
        return Steer();
    }

    //-------------------------------------------------------------
    // Objective arrival complete
    //-------------------------------------------------------------

    if (bObjectiveArrived)
    {
        if (navpt)
        {
            ship->SetNavptStatus(
                navpt,
                INSTRUCTION_STATUS::COMPLETE);
        }

        return Steer();
    }

    //-------------------------------------------------------------
    // FARCASTER / QUANTUM navpoint processing
    //-------------------------------------------------------------

    if (navpt)
    {
        SimRegion* self_rgn =
            ship->GetRegion();

        SimRegion* nav_rgn =
            navpt->GetRegion();

        QuantumDrive* qdrive =
            ship->GetQuantumDrive();

        if (self_rgn &&
            !nav_rgn)
        {
            nav_rgn =
                self_rgn;

            navpt->SetRegion(
                nav_rgn);
        }

        const bool bFarcasterNav =
            navpt->GetFarcast() != 0;

        const bool use_farcaster =
            bFarcasterNav ||
            (self_rgn != nav_rgn &&
                (!qdrive ||
                    !qdrive->IsPowerOn() ||
                    qdrive->GetStatus() <
                    SYSTEM_STATUS::DEGRADED));

        if (use_farcaster)
        {
            if (!farcaster &&
                self_rgn)
            {
                ListIter<Ship> S =
                    self_rgn->GetShips();

                while (++S &&
                    !farcaster)
                {
                    Ship* Candidate =
                        S.value();

                    if (!Candidate)
                    {
                        continue;
                    }

                    Farcaster* FC =
                        Candidate->GetFarcaster();

                    if (!FC)
                    {
                        continue;
                    }

                    const Ship* Dest =
                        FC->GetDest();

                    if (bFarcasterNav)
                    {
                        farcaster =
                            FC;
                    }
                    else if (Dest &&
                        Dest->GetRegion() == nav_rgn)
                    {
                        farcaster =
                            FC;
                    }
                }
            }

            if (farcaster)
            {
                if (!bFarcasterNav &&
                    farcaster->GetShip() &&
                    farcaster->GetShip()->GetRegion() != self_rgn &&
                    farcaster->GetDest())
                {
                    farcaster =
                        farcaster->GetDest()->GetFarcaster();
                }

                obj_w =
                    farcaster->EndPoint();

                distance =
                    (obj_w -
                        ship->GetLocation()).Size();

                objective =
                    Transform(obj_w);
            }
        }
        else if (self_rgn != nav_rgn)
        {
            QuantumDrive* Q =
                ship->GetQuantumDrive();

            if (Q &&
                Q->ActiveState() ==
                QuantumDrive::ACTIVE_READY)
            {
                Q->SetDestination(
                    navpt->GetRegion(),
                    navpt->GetLocation());

                Q->Engage();
            }
        }
    }

    return ShipAI::SeekTarget();
}

// +----------------------------------------------------------------------+

Steer
StarshipAI::AvoidCollision()
{
    if (!ship ||
        ship->GetVelocity().Size() < 25.0)
    {
        return Steer();
    }

    return ShipAI::AvoidCollision();
}

// +----------------------------------------------------------------------+

void
StarshipAI::FireControl()
{
    //-------------------------------------------------------------
    // Identify unknown contacts
    //-------------------------------------------------------------
    if (identify)
    {
        if (FMath::Abs(
            ship->GetHelmHeading() -
            ship->GetCompassHeading()) <
            10.0 * DEGREES)
        {
            SimContact* Contact =
                ship->FindContact(target);

            if (Contact &&
                !Contact->ActLock())
            {
                if (!ship->GetProbe())
                {
                    ship->LaunchProbe();
                }
            }
        }

        return;
    }

    //-------------------------------------------------------------
    // Investigate last known enemy location
    //-------------------------------------------------------------
    if (rumor &&
        !target &&
        ship->GetProbeLauncher() &&
        !ship->GetProbe())
    {
        FVector Rmr =
            Transform(rumor->GetLocation());

        Rmr.Normalize();

        const double dx =
            FMath::Abs(Rmr.X);

        const double dy =
            FMath::Abs(Rmr.Y);

        if (dx < 10.0 * DEGREES &&
            dy < 10.0 * DEGREES &&
            Rmr.Z > 0.0)
        {
            ship->LaunchProbe();
        }
    }

    //-------------------------------------------------------------
    // Corvettes / Frigates: anti-air platforms
    //-------------------------------------------------------------
    if (ship->GetClassification() == CLASSIFICATION::CORVETTE ||
        ship->GetClassification() == CLASSIFICATION::FRIGATE)
    {
        ListIter<WeaponGroup> Iter =
            ship->GetWeapons();

        while (++Iter)
        {
            WeaponGroup* Group =
                Iter.value();

            ListIter<Weapon> WIter =
                Group->GetWeapons();

            while (++WIter)
            {
                Weapon* W =
                    WIter.value();

                const double Az =
                    W->GetAzimuth();

                if (FMath::Abs(Az) <
                    45.0 * DEGREES)
                {
                    W->SetFiringOrders(
                        WeaponsOrders::AUTO);

                    W->SetTarget(
                        target,
                        nullptr);
                }
                else
                {
                    W->SetFiringOrders(
                        WeaponsOrders::POINT_DEFENSE);
                }
            }
        }
    }

    //-------------------------------------------------------------
    // Other starships: anti-ship fire control
    //-------------------------------------------------------------
    else
    {
        SimSystem* Subtgt =
            SelectSubtarget();

        ListIter<WeaponGroup> Iter =
            ship->GetWeapons();

        while (++Iter)
        {
            WeaponGroup* Group =
                Iter.value();

            const int32 DropShipMask =
                static_cast<int32>(CLASSIFICATION::DROPSHIPS);

            if (Group->GetDesign()->target_type &
                DropShipMask)
            {
                Group->SetFiringOrders(
                    WeaponsOrders::POINT_DEFENSE);
            }
            else if (Group->IsDrone())
            {
                Group->SetFiringOrders(
                    WeaponsOrders::MANUAL);

                Group->SetTarget(
                    target,
                    nullptr);

                if (target &&
                    target->GetRegion() ==
                    ship->GetRegion())
                {
                    FVector Delta =
                        target->GetLocation() -
                        ship->GetLocation();

                    const double Range =
                        Delta.Size();

                    if (Range <
                        Group->GetDesign()->max_range * 0.9 &&
                        !AssessTargetPointDefense())
                    {
                        Group->SetFiringOrders(
                            WeaponsOrders::AUTO);
                    }
                    else if (Range <
                        Group->GetDesign()->max_range * 0.5)
                    {
                        Group->SetFiringOrders(
                            WeaponsOrders::AUTO);
                    }
                }
            }
            else
            {
                Group->SetFiringOrders(
                    WeaponsOrders::AUTO);

                Group->SetTarget(
                    target,
                    Subtgt);

                Group->SetSweep(
                    Subtgt
                    ? WeaponsSweep::SWEEP_NONE
                    : WeaponsSweep::SWEEP_TIGHT);
            }
        }
    }
}

// +----------------------------------------------------------------------+

SimSystem*
StarshipAI::SelectSubtarget()
{
    if (Game::GetGameTime() -
        sub_select_time <
        2345)
    {
        return subtarget;
    }

    subtarget =
        nullptr;

    if (!target ||
        target->GetType() != ESimObject::SHIP ||
        GetAILevel() < 1)
    {
        return subtarget;
    }

    Ship* TargetShip =
        static_cast<Ship*>(target);

    if (!TargetShip->IsStarship())
    {
        return subtarget;
    }

    Weapon* Subtgt =
        nullptr;

    double Dist =
        50e3;

    FVector SVec =
        ship->GetLocation() -
        TargetShip->GetLocation();

    sub_select_time =
        Game::GetGameTime();

    //-------------------------------------------------------------
    // First pass: turrets
    //-------------------------------------------------------------
    ListIter<WeaponGroup> GIter =
        TargetShip->GetWeapons();

    while (++GIter)
    {
        WeaponGroup* Group =
            GIter.value();

        if (Group->GetDesign() &&
            Group->GetDesign()->turret_model)
        {
            ListIter<Weapon> WIter =
                Group->GetWeapons();

            while (++WIter)
            {
                Weapon* W =
                    WIter.value();

                if (W->GetAvailability() < 35.0)
                {
                    continue;
                }

                if (FVector::DotProduct(
                    W->GetAimVector(),
                    SVec) < 0.0)
                {
                    continue;
                }

                if (W->GetTurret())
                {
                    FVector TLoc =
                        W->GetTurret()->Location();

                    FVector Delta =
                        TLoc -
                        ship->GetLocation();

                    const double DLen =
                        Delta.Size();

                    if (DLen < Dist)
                    {
                        Subtgt =
                            W;

                        Dist =
                            DLen;
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------
    // Second pass: major weapons
    //-------------------------------------------------------------
    if (!Subtgt)
    {
        GIter.reset();

        while (++GIter)
        {
            WeaponGroup* Group =
                GIter.value();

            if (Group->GetDesign() &&
                !Group->GetDesign()->turret_model)
            {
                ListIter<Weapon> WIter =
                    Group->GetWeapons();

                while (++WIter)
                {
                    Weapon* W =
                        WIter.value();

                    if (W->GetAvailability() < 35.0)
                    {
                        continue;
                    }

                    if (FVector::DotProduct(
                        W->GetAimVector(),
                        SVec) < 0.0)
                    {
                        continue;
                    }

                    FVector TLoc =
                        W->GetMountLocation();

                    FVector Delta =
                        TLoc -
                        ship->GetLocation();

                    const double DLen =
                        Delta.Size();

                    if (DLen < Dist)
                    {
                        Subtgt =
                            W;

                        Dist =
                            DLen;
                    }
                }
            }
        }
    }

    subtarget =
        Subtgt;

    return subtarget;
}

// +----------------------------------------------------------------------+

bool
StarshipAI::AssessTargetPointDefense()
{
    if (Game::GetGameTime() -
        point_defense_time <
        3500)
    {
        return tgt_point_defense;
    }

    tgt_point_defense =
        false;

    if (!target ||
        target->GetType() != ESimObject::SHIP ||
        GetAILevel() < 2)
    {
        return tgt_point_defense;
    }

    Ship* TargetShip =
        static_cast<Ship*>(target);

    if (!TargetShip->IsStarship())
    {
        return tgt_point_defense;
    }

    FVector SVec =
        ship->GetLocation() -
        TargetShip->GetLocation();

    point_defense_time =
        Game::GetGameTime();

    ListIter<WeaponGroup> GIter =
        TargetShip->GetWeapons();

    while (++GIter &&
        !tgt_point_defense)
    {
        WeaponGroup* Group =
            GIter.value();

        if (Group->CanTarget(1))
        {
            ListIter<Weapon> WIter =
                Group->GetWeapons();

            while (++WIter &&
                !tgt_point_defense)
            {
                Weapon* W =
                    WIter.value();

                if (W->GetAvailability() > 35.0 &&
                    FVector::DotProduct(
                        W->GetAimVector(),
                        SVec) > 0.0)
                {
                    tgt_point_defense =
                        true;
                }
            }
        }
    }

    return tgt_point_defense;
}

// +----------------------------------------------------------------------+

FVector
StarshipAI::Transform(const FVector& Point)
{
    if (!self)
    {
        return FVector::ZeroVector;
    }

    // LEGACY STARSHIPAI:
    // target relative to ship
    return Point - self->GetLocation();
}

// +----------------------------------------------------------------------+

Steer
StarshipAI::Seek(const FVector& Point)
{
    //-------------------------------------------------------------
    // Legacy:
    // Point is relative world coordinates.
    //
    // X = lateral/right-left
    // Y = altitude/up-down
    // Z = forward/back
    //-------------------------------------------------------------
    Steer Result;

    Result.yaw =
        FMath::Atan2(
            Point.X,
            Point.Z);

    const double Adjacent =
        FMath::Sqrt(
            Point.X * Point.X +
            Point.Z * Point.Z);

    if (ship &&
        FMath::Abs(Point.Y) > ship->GetRadius() &&
        Adjacent > ship->GetRadius())
    {
        Result.pitch =
            FMath::Atan2(
                Point.Y,
                Adjacent);
    }

    if (!FMath::IsFinite(Result.yaw))
    {
        Result.yaw = 0.0;
    }

    if (!FMath::IsFinite(Result.pitch))
    {
        Result.pitch = 0.0;
    }

    if (ship &&
        !_stricmp(ship->GetName(), "Blockade Runner"))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[StarshipAI::Seek BR] ")
            TEXT("Point=%s Yaw=%.4f Pitch=%.4f"),
            *Point.ToString(),
            Result.yaw,
            Result.pitch);
    }

    return Result;
}

// +----------------------------------------------------------------------+

Steer
StarshipAI::Flee(const FVector& Point)
{
    Steer Result =
        Seek(Point);

    Result.yaw +=
        PI;

    return Result;
}

// +----------------------------------------------------------------------+

Steer
StarshipAI::Avoid(
    const FVector& Point,
    float Radius)
{
    Steer Result =
        Seek(Point);

    if (!ship)
    {
        return Result;
    }

    if (FVector::DotProduct(
        Point,
        ship->GetBeamLine()) > 0.0)
    {
        Result.yaw -=
            PI / 2.0;
    }
    else
    {
        Result.yaw +=
            PI / 2.0;
    }

    return Result;
}

void
StarshipAI::DebugThrottleControl(
    const char* Phase,
    double ShipSpeed,
    double InThrottle,
    double InBrakes,
    double ObjectiveDistance,
    double TargetDistance,
    double BrakeAlpha,
    double EasedBrakeAlpha) const
{
    if (!ship)
    {
        return;
    }

    if (_stricmp(ship->GetName(), "Blockade Runner"))
    {
        return;
    }

    UE_LOG(LogTemp, Error,
        TEXT("[StarshipAI::ThrottleControl BR] ")
        TEXT("Phase='%hs' ")
        TEXT("ObjDist=%.2f ")
        TEXT("TargetDist=%.2f ")
        TEXT("Target='%hs' ")
        TEXT("ShipSpeed=%.2f ")
        TEXT("Throttle=%.2f ")
        TEXT("Brakes=%.2f ")
        TEXT("BrakeAlpha=%.3f ")
        TEXT("EasedBrakeAlpha=%.3f"),
        Phase ? Phase : "NULL",
        ObjectiveDistance,
        TargetDistance,
        target ? target->GetName() : "NULL",
        ShipSpeed,
        InThrottle,
        InBrakes,
        BrakeAlpha,
        EasedBrakeAlpha);
}