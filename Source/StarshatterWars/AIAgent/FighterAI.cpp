/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         FighterAI.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Fighter (low-level) Artificial Intelligence class
*/


#include "FighterAI.h"
#include "CoreMinimal.h"
#include "FighterTacticalAI.h"
#include "Ship.h"
#include "SimShot.h"
#include "Sensor.h"
#include "SimElement.h"
#include "ShipDesign.h"
#include "Instruction.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Farcaster.h"
#include "FlightComputer.h"
#include "FlightDeck.h"
#include "Hangar.h"
#include "Sim.h"
#include "StarSystem.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"

#include "Game.h"

#include "CoreMinimal.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "GameStructs.h"

static const double TIME_TO_DOCK = 30.0;

// +----------------------------------------------------------------------+

FighterAI::FighterAI(SimObject* s)
    : ShipAI(s),
    brakes(0.0),
    drop_state(0),
    jink_time(0),
    evading(false),
    decoy_missile(0),
    missile_time(0.0),
    terrain_warning(false),
    inbound(0),
    rtb_code(0),
    form_up(false),
    over_threshold(false),
    time_to_dock(0.0),
    go_manual(false)
{
    ai_type = FIGHTER;
    seek_gain = 22.0;
    seek_damp = 0.55;
    brakes = 0.0;
    z_shift = 0.0;

    tactical = new FighterTacticalAI(this);
}

// +--------------------------------------------------------------------+

FighterAI::~FighterAI()
{
}

// +--------------------------------------------------------------------+

static double frame_time = 0.0;

void
FighterAI::ExecFrame(double s)
{
    if (!ship)
        return;

    evading = false;
    inbound = ship->GetInbound();
    missile_time -= s;

    RadioMessageAction order = RadioMessageAction::NONE;

    if (navpt)
        order = navpt->GetRadioAction();

    if (inbound) {
        form_up = false;
        rtb_code = 1;

        // CHEAT LANDING:
        if (inbound->Final() && time_to_dock > 0) {
            FlightDeck* deck = inbound->GetDeck();
            if (deck) {
                const FVector Dst = deck->EndPoint();
                const FVector Approach = deck->StartPoint() - Dst;

                const Ship* carrier = deck->GetCarrier();

                Camera landing_cam;
                landing_cam.Clone(carrier->Cam());
                landing_cam.Yaw(deck->Azimuth());

                if (time_to_dock > TIME_TO_DOCK / 2.0) {
                    double lr, lp, lw;
                    double sr, sp, sw;

                    landing_cam.Orientation().ComputeEulerAngles(lr, lp, lw);
                    ship->Cam().Orientation().ComputeEulerAngles(sr, sp, sw);

                    const double nr = sr + s * (lr - sr);
                    const double np = sp + s * (lp - sp);
                    const double nw = sw + s * (lw - sw) * 0.5;

                    Camera work;
                    work.Aim(nr, np, nw);
                    landing_cam.Clone(work);
                }

                ship->CloneCam(landing_cam);

                // NOTE: Starshatter original used OtherHand() for coordinate handedness conversion.
                // In UE, this should already be handled by your coordinate convention. If you still
                // need the conversion, apply it here (or centralize in a helper).
                ship->MoveTo(Dst + Approach * (time_to_dock / TIME_TO_DOCK));

                ship->SetVelocity(carrier->Velocity() + ship->Heading() * 50.0);
                ship->SetThrottle(50);
                ship->ExecFLCSFrame();

                time_to_dock -= s;

                if (time_to_dock <= 0) {
                    deck->Dock(ship);
                    time_to_dock = 0;
                }

                return;
            }
        }

        else if (ship->GetFlightPhase() == Ship::DOCKING) {
            // deal with (pathological) moving carrier deck:

            FlightDeck* deck = inbound->GetDeck();
            if (deck) {
                FVector dst = deck->EndPoint();

                if (ship->IsAirborne()) {
                    const double alt = dst.Y;
                    dst = ship->Location();
                    dst.Y = alt;
                }

                const Ship* carrier = deck->GetCarrier();

                Camera landing_cam;
                landing_cam.Clone(carrier->Cam());
                landing_cam.Yaw(deck->Azimuth());

                ship->CloneCam(landing_cam);
                ship->MoveTo(dst);

                if (!ship->IsAirborne()) {
                    ship->SetVelocity(carrier->Velocity());
                }
                else {
                    const FVector taxi(landing_cam.vpn());
                    ship->SetVelocity(taxi * 95.0);
                }

                ship->SetThrottle(0);
                ship->ExecFLCSFrame();
            }

            return;
        }
    }
    else {
        Instruction* orders = ship->GetRadioOrders();

        if (orders &&
            (orders->GetRadioAction() == RadioMessageAction::WEP_HOLD ||
                orders->GetRadioAction() == RadioMessageAction::FORM_UP)) {
            form_up = true;
            rtb_code = 0;
        }
        else {
            form_up = false;
        }
    }

    const INSTRUCTION_ACTION InstrOrder =
        static_cast<INSTRUCTION_ACTION>(static_cast<int32>(order));

    if (!target && InstrOrder != INSTRUCTION_ACTION::STRIKE)
        ship->SetSensorMode(Sensor::STD);

    ShipAI::ExecFrame(s); // this must be the last line of this method

    // IT IS NOT SAFE TO PLACE CODE HERE
    // if this class decides to break orbit,
    // this object will be deleted during
    // ShipAI::ExecFrame() (which calls
    // FighterAI::Navigator() - see below)
}

// +--------------------------------------------------------------------+

void
FighterAI::FindObjective()
{
    distance = 0;

    // ALWAYS complete initial launch navpt:
    if (!navpt) {
        navpt = ship->GetNextNavPoint();
        if (navpt && (navpt->GetAction() != INSTRUCTION_ACTION::LAUNCH || navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE))
            navpt = 0;
    }

    if (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH) {
        if (navpt->GetStatus() != INSTRUCTION_STATUS::COMPLETE) {
            FindObjectiveNavPoint();

            // transform into camera coords:
            objective = Transform(obj_w);
            ship->SetDirectorInfo("Launch");
            return;
        }
        else {
            navpt = 0;
        }
    }

    // runway takeoff:
    else if (takeoff) {
        obj_w = ship->Location() + ship->Heading() * 10e3;
        obj_w.Y = ship->Location().Y + 2e3;

        // transform into camera coords:
        objective = Transform(obj_w);
        ship->SetDirectorInfo("Takeoff");
        return;
    }

    // approaching a carrier or runway:
    else if (inbound) {
        FlightDeck* deck = inbound->GetDeck();

        if (!deck) {
            objective = FVector::ZeroVector;
            return;
        }

        // initial approach
        if (inbound->Approach() > 0 || !inbound->Cleared()) {
            obj_w = deck->ApproachPoint(inbound->Approach()) + inbound->Offset();

            distance = (obj_w - ship->Location()).Size();

            // transform into camera coords:
            objective = Transform(obj_w);
            ship->SetDirectorInfo("Inbound");

            return;
        }

        // final approach
        else {
            ship->SetDirectorInfo("Finals");

            obj_w = deck->StartPoint();
            if (inbound->Final()) {
                obj_w = deck->EndPoint();

                if (deck->OverThreshold(ship)) {
                    obj_w = deck->MountLocation();
                    over_threshold = true;
                }
            }

            distance = (obj_w - ship->Location()).Size();

            // transform into camera coords:
            objective = Transform(obj_w);

            return;
        }
    }

    // not inbound yet, check for RTB order:
    else {
        Instruction* orders = (Instruction*)ship->GetRadioOrders();
        RadioMessageAction action = RadioMessageAction::NONE;

        if (orders)
            action = orders->GetRadioAction();

        if (navpt && action != RadioMessageAction::NONE) {
            FindObjectiveNavPoint();
            if (distance < 5e3) {
                action = navpt->GetRadioAction();
            }
        }

        if (action == RadioMessageAction::RTB ||
            action == RadioMessageAction::DOCK_WITH) {

            Ship* controller = ship->GetController();

            if (orders && orders->GetRadioAction() == RadioMessageAction::DOCK_WITH && orders->GetTarget()) {
                controller = (Ship*)orders->GetTarget();
            }

            else if (navpt && navpt->GetRadioAction() == RadioMessageAction::DOCK_WITH && navpt->GetTarget()) {
                controller = (Ship*)navpt->GetTarget();
            }

            ReturnToBase(controller);

            if (rtb_code)
                return;
        }
    }

    ShipAI::FindObjective();
}

void
FighterAI::ReturnToBase(Ship* controller)
{
    rtb_code = 0;

    if (controller) {
        SimRegion* self_rgn = ship->GetRegion();
        SimRegion* rtb_rgn = controller->GetRegion();

        if (self_rgn && !rtb_rgn) {
            rtb_rgn = self_rgn;
        }

        if (self_rgn && rtb_rgn && self_rgn != rtb_rgn) {
            // is the carrier in orbit above us
            // (or on the ground below us)?

            if (rtb_rgn->GetOrbitalRegion()->Primary() ==
                self_rgn->GetOrbitalRegion()->Primary()) {

                FVector npt = rtb_rgn->GetLocation() - self_rgn->GetLocation();
                obj_w = npt; // NOTE: Original used OtherHand().

                // distance from self to navpt:
                distance = (obj_w - ship->Location()).Size();

                // transform into camera coords:
                objective = Transform(obj_w);

                if (rtb_rgn->IsAirSpace()) {
                    drop_state = -1;
                }
                else if (rtb_rgn->IsOrbital()) {
                    drop_state = 1;
                }

                rtb_code = 2;
            }

            // try to find a jumpgate that will take us home:
            else {
                QuantumDrive* qdrive = ship->GetQuantumDrive();
                const bool    use_farcaster = !qdrive ||
                    !qdrive->IsPowerOn() ||
                    qdrive->GetStatus() < SYSTEM_STATUS::DEGRADED;

                if (use_farcaster) {
                    if (!farcaster) {
                        ListIter<Ship> s = self_rgn->GetShips();
                        while (++s && !farcaster) {
                            if (s->GetFarcaster()) {
                                const Ship* dest = s->GetFarcaster()->GetDest();
                                if (dest && dest->GetRegion() == rtb_rgn) {
                                    farcaster = s->GetFarcaster();
                                }
                            }
                        }
                    }

                    if (farcaster) {
                        const FVector apt = farcaster->ApproachPoint(0);
                        const FVector npt = farcaster->StartPoint();
                        const double  r1 = (ship->Location() - npt).Size();

                        if (r1 > 50e3) {
                            obj_w = apt;
                            distance = r1;
                            objective = Transform(obj_w);
                        }

                        else {
                            const double r2 = (ship->Location() - apt).Size();
                            const double r3 = (npt - apt).Size();

                            if (r1 + r2 < 1.2 * r3) {
                                obj_w = npt;
                                distance = r1;
                                objective = Transform(obj_w);
                            }
                            else {
                                obj_w = apt;
                                distance = r2;
                                objective = Transform(obj_w);
                            }
                        }

                        rtb_code = 3;
                    }

                    // can't find a way back home, ignore the RTB order:
                    else {
                        ship->ClearRadioOrders();
                        rtb_code = 0;
                        return;
                    }
                }
                else if (qdrive) {
                    if (qdrive->ActiveState() == QuantumDrive::ACTIVE_READY) {
                        qdrive->SetDestination(rtb_rgn, controller->Location());
                        qdrive->Engage();
                    }

                    rtb_code = 3;
                }
            }
        }

        else {
            obj_w = controller->Location();

            distance = (obj_w - ship->Location()).Size();

            // transform into camera coords:
            objective = Transform(obj_w);
            ship->SetDirectorInfo(Game::GetText("ai.return-to-base"));

            rtb_code = 1;
        }
    }
}

// +--------------------------------------------------------------------+

void
FighterAI::FindObjectiveNavPoint()
{
    SimRegion* self_rgn = ship->GetRegion();
    SimRegion* nav_rgn = navpt->Region();

    if (self_rgn && !nav_rgn) {
        nav_rgn = self_rgn;
        navpt->SetRegion(nav_rgn);
    }

    if (self_rgn && nav_rgn && self_rgn != nav_rgn) {
        if (nav_rgn->GetOrbitalRegion()->Primary() ==
            self_rgn->GetOrbitalRegion()->Primary()) {

            FVector npt = nav_rgn->GetLocation() - self_rgn->GetLocation();
            obj_w = npt; // NOTE: Original used OtherHand().

            // distance from self to navpt:
            distance = (obj_w - ship->Location()).Size();

            // transform into camera coords:
            objective = Transform(obj_w);

            if (nav_rgn->IsAirSpace()) {
                drop_state = -1;
            }
            else if (nav_rgn->IsOrbital()) {
                drop_state = 1;
            }

            return;
        }

        else {
            QuantumDrive* q = ship->GetQuantumDrive();

            if (q) {
                if (q->ActiveState() == QuantumDrive::ACTIVE_READY) {
                    q->SetDestination(navpt->Region(), navpt->Location());
                    q->Engage();
                    return;
                }
            }
        }
    }

    ShipAI::FindObjectiveNavPoint();
}

// +--------------------------------------------------------------------+

FVector
FighterAI::ClosingVelocity()
{
    if (ship) {
        WeaponDesign* wep_design = ship->GetPrimaryDesign();

        if (target && wep_design) {
            FVector aim_vec = ship->Heading().GetSafeNormal();
            const FVector shot_vel = ship->Velocity() + aim_vec * wep_design->speed;
            return shot_vel - target->Velocity();
        }

        else if (target) {
            return ship->Velocity() - target->Velocity();
        }

        else {
            return ship->Velocity();
        }
    }

    return FVector(1, 0, 0);
}

// +--------------------------------------------------------------------+

void
FighterAI::Navigator()
{
    go_manual = false;

    if (takeoff) {
        accumulator.Clear();
        magnitude = 0;
        brakes = 0;
        z_shift = 0;

        Accumulate(SeekTarget());
        HelmControl();
        ThrottleControl();
        ship->ExecFLCSFrame();
        return;
    }

    SimElement* elem = ship->GetElement();

    if (elem) {
        Ship* lead = elem->GetShip(1);

        if (lead && lead != ship) {
            if (lead->IsDropping() && !ship->IsDropping()) {
                ship->DropOrbit();
                // careful: this object has just been deleted!
                return;
            }

            if (lead->IsAttaining() && !ship->IsAttaining()) {
                ship->MakeOrbit();
                // careful: this object has just been deleted!
                return;
            }
        }

        else {
            if (drop_state < 0) {
                ship->DropOrbit();
                // careful: this object has just been deleted!
                return;
            }

            if (drop_state > 0) {
                ship->MakeOrbit();
                // careful: this object has just been deleted!
                return;
            }
        }
    }

    INSTRUCTION_ACTION order = INSTRUCTION_ACTION::VECTOR;

    if (navpt)
        order = navpt->GetAction();

    if (rtb_code == 1 && navpt && navpt->GetStatus() < INSTRUCTION_STATUS::SKIPPED &&
        !inbound && distance < 35e3) { // (this should be distance to the ship)

        if (order == INSTRUCTION_ACTION::RTB) {
            Ship* controller = ship->GetController();
            Hangar* hangar = controller ? controller->GetHangar() : 0;

            if (hangar && hangar->CanStow(ship)) {
                for (int i = 0; i < elem->NumShips(); i++) {
                    Ship* s = elem->GetShip(i + 1);

                    if (s && s->GetDirector() && s->GetDirector()->Type() >= ShipAI::FIGHTER)
                        RadioTraffic::SendQuickMessage(s, RadioMessageAction::CALL_INBOUND);
                }

                if (element_index == 1)
                    ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
            }

            else {
                if (element_index == 1) {
                    UE_LOG(LogTemp, Warning,
                        TEXT("WARNING: FighterAI NAVPT RTB, but no controller or hangar found for ship '%s'"),
                        ship ? ANSI_TO_TCHAR(ship->Name()) : TEXT("null"));
                    ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::SKIPPED);
                }
            }
        }

        else {
            Ship* dock_target = (Ship*)navpt->GetTarget();
            if (dock_target) {
                for (int i = 0; i < elem->NumShips(); i++) {
                    Ship* s = elem->GetShip(i + 1);

                    if (s) {
                        RadioMessage* msg = new RadioMessage(dock_target, s, RadioMessageAction::CALL_INBOUND);
                        RadioTraffic::Transmit(msg);
                    }
                }

                if (element_index == 1)
                    ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
            }

            else {
                if (element_index == 1) {
                    UE_LOG(LogTemp, Warning,
                        TEXT("WARNING: FighterAI NAVPT DOCK, but no dock target found for ship '%s'"),
                        ship ? ANSI_TO_TCHAR(ship->Name()) : TEXT("null"));
                    ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::SKIPPED);
                }
            }
        }
    }

    if (target)
        ship->SetDirectorInfo("Seek Target");

    accumulator.Clear();
    magnitude = 0;
    brakes = 0;
    z_shift = 0;

    hold = false;
    if ((ship->GetElement() && ship->GetElement()->GetHoldTime() > 0) ||
        (navpt && navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt->HoldTime() > 0))
        hold = true;

    if (ship->MissionClock() < 10000) {
        if (ship->IsAirborne())
            Accumulate(SeekTarget());
    }

    else if ((farcaster && distance < 20e3) || (inbound && inbound->Final())) {
        Accumulate(SeekTarget());
    }

    else {
        if (!ship->IsAirborne() || ship->AltitudeAGL() > 100)
            ship->RaiseGear();

        Accumulate(AvoidTerrain());
        Steer avoid = AvoidCollision();

        if (other && inbound && inbound->GetDeck() && inbound->Cleared()) {
            if (other != (SimObject*)inbound->GetDeck()->GetCarrier())
                Accumulate(avoid);
        }
        else {
            Accumulate(avoid);
        }

        if (!too_close && !hold && !terrain_warning) {
            Accumulate(SeekTarget());
            Accumulate(EvadeThreat());
        }
    }

    HelmControl();
    ThrottleControl();
    FireControl();
    AdjustDefenses();

    ship->ExecFLCSFrame();
}

// +--------------------------------------------------------------------+

void
FighterAI::HelmControl()
{
    Camera* cam = ((Camera*)&(ship->Cam()));
    FVector vrt = cam->vrt();
    double  deflection = vrt.Y;
    double  theta = 0;
    bool    formation = element_index > 1;
    bool    station_keeping = distance < 0;
    bool    inverted = cam->vup().Y < -0.5;
    Ship* ward = ship->GetWard();

    if (takeoff || inbound || station_keeping)
        formation = false;

    if (takeoff || navpt || farcaster || patrol || inbound || rtb_code || target || ward || threat || formation) {
        // are we being asked to flee?
        if (FMath::Abs(accumulator.yaw) == 1.0 && accumulator.pitch == 0.0) {
            accumulator.pitch = -0.7f;
            accumulator.yaw *= 0.25f;

            if (ship->IsAirborne() && ship->GetFlightModel() == 0)
                accumulator.pitch = -0.45f;

            // low ai -> lower turning rate
            accumulator.pitch += 0.1f * (2 - ai_level);
        }

        ship->ApplyRoll((float)(accumulator.yaw * -0.7));
        ship->ApplyYaw((float)(accumulator.yaw * 0.2));

        if (FMath::Abs(accumulator.yaw) > 0.5 && FMath::Abs(accumulator.pitch) < 0.1)
            accumulator.pitch -= 0.1f;

        ship->ApplyPitch((float)accumulator.pitch);
    }

    else {
        ship->SetDirectorInfo(Game::GetText("ai.station-keeping"));
        station_keeping = true;

        // go into a slow orbit if airborne:
        if (ship->IsAirborne() && ship->Class() < CLASSIFICATION::LCA) {
            accumulator.brake = 0.2;
            accumulator.stop = 0;

            const double compass_pitch = ship->CompassPitch();
            const double desired_bank = -PI / 4;
            const double current_bank = asin(deflection);
            const double theta_local = desired_bank - current_bank;
            ship->ApplyRoll(theta_local);

            const double coord_pitch = compass_pitch - 0.2 * FMath::Abs(current_bank);
            ship->ApplyPitch(coord_pitch);
        }
        else {
            accumulator.brake = 1;
            accumulator.stop = 1;
        }
    }

    // if not turning, roll to orient with world coords:
    if (ship->Design()->auto_roll > 0) {
        if (FMath::Abs(accumulator.pitch) < 0.1 && FMath::Abs(accumulator.yaw) < 0.25) {
            // zolon spiral behavior:
            if (ship->Design()->auto_roll > 1) {
                if ((element_index + (ship->MissionClock() >> 10)) & 0x4)
                    ship->ApplyRoll(0.60);
                else
                    ship->ApplyRoll(-0.35);
            }

            // normal behavior - roll to upright:
            else if (FMath::Abs(deflection) > 0.1 || inverted) {
                const double theta_roll = asin(deflection / vrt.Size()) * 0.5;
                ship->ApplyRoll(-theta_roll);
            }
        }
    }

    // if not otherwise occupied, pitch to orient with world coords:
    if (station_keeping && (!ship->IsAirborne() || ship->Class() < CLASSIFICATION::LCA)) {
        const FVector heading = ship->Heading();
        const double  pitch_deflection = heading.Y;

        if (FMath::Abs(pitch_deflection) > 0.05) {
            const double rho = asin(pitch_deflection) * 3;
            ship->ApplyPitch(rho);
        }
    }

    ship->SetTransX(0);
    ship->SetTransY(0);
    ship->SetTransZ(z_shift * ship->Design()->trans_z);
    ship->SetFLCSMode(go_manual ? Ship::FLCS_MANUAL : Ship::FLCS_AUTO);
}

void
FighterAI::ThrottleControl()
{
    SimElement* elem = ship ? ship->GetElement() : nullptr;
    const double ship_speed = ship ? (ship->Velocity() | ship->Heading()) : 0.0; // dot product
    double desired = 1000.0;
    bool formation = element_index > 1;
    const bool station_keeping = distance < 0.0;
    bool augmenter = false;
    Ship* ward = ship ? ship->GetWard() : nullptr;

    if (!ship)
        return;

    if (inbound || station_keeping)
        formation = false;

    // LAUNCH / TAKEOFF
    if (ship->MissionClock() < 10000) {
        formation = false;
        throttle = 100.0;
        brakes = 0.0;
    }

    // STATION KEEPING
    else if (station_keeping) {
        // go into a slow orbit if airborne:
        if (ship->IsAirborne() && ship->Class() < CLASSIFICATION::LCA) {
            throttle = 30.0;
            brakes = 0.0;
        }
        else {
            throttle = 0.0;
            brakes = 1.0;
        }
    }

    // TRY TO STAY AIRBORNE
    else if (ship->IsAirborne() && ship_speed < 250.0 && ship->Class() < CLASSIFICATION::LCA) {
        throttle = 100.0;
        brakes = 0.0;

        if (ship_speed < 200.0)
            augmenter = true;
    }

    // INBOUND
    else if (inbound) {
        const double carrier_speed = inbound->GetDeck()->GetCarrier()->Velocity().Size();
        desired = 250.0 + carrier_speed;

        if (distance > 25.0e3)
            desired = 750.0 + carrier_speed;

        else if (ship->IsAirborne())
            desired = 300.0;

        else if (inbound->Final())
            desired = 75.0 + carrier_speed;

        throttle = 0.0;

        // holding short?
        if (inbound->Approach() == 0 && !inbound->Cleared() &&
            distance < 2000.0 && !ship->IsAirborne()) {
            desired = 0.0;
        }

        if (ship_speed > desired + 5.0)
            brakes = 0.25;

        else if (ship->IsAirborne() || Ship::GetFlightModel() > 0) {
            throttle = old_throttle + 1.0;
        }

        else if (ship_speed < 0.85 * desired) {
            throttle = 100.0;

            if (ship_speed < 0.0 && ship->GetFuelLevel() > 10.0)
                augmenter = true;
        }

        else if (ship_speed < desired - 5.0) {
            throttle = 30.0;
        }
    }

    // RTB / FARCASTER
    else if (rtb_code || farcaster) {
        desired = 750.0;

        if (threat || threat_missile) {
            throttle = 100.0;

            if (!threat_missile && ship->GetFuelLevel() > 15.0)
                augmenter = true;
        }

        else {
            throttle = 0.0;

            if (ship_speed > desired + 5.0)
                brakes = 0.25;

            else if (Ship::GetFlightModel() > 0) {
                throttle = old_throttle + 1.0;
            }

            else if (ship_speed < 0.85 * desired) {
                throttle = 100.0;

                if (ship_speed < 0.0 && ship->GetFuelLevel() > 10.0)
                    augmenter = true;
            }

            else if (ship_speed < desired - 5.0) {
                throttle = 30.0;
            }
        }
    }

    // RUN AWAY
    else if (evading) {
        throttle = 100.0;

        if (!threat_missile && ship->GetFuelLevel() > 15.0)
            augmenter = true;
    }

    // PATROL AND FORMATION
    else if (!navpt && !target && !ward) {
        if (!elem || !formation) { // element lead
            if (patrol) {
                desired = 250.0;

                if (distance > 10e3)
                    desired = 750.0;

                if (ship_speed > desired + 5.0) {
                    brakes = 0.25;
                    throttle = old_throttle - 5.0;
                }

                else if (ship_speed < 0.85 * desired) {
                    throttle = 100.0;

                    if (ship_speed < 0.0 && ship->GetFuelLevel() > 10.0)
                        augmenter = true;
                }

                else if (ship_speed < desired - 5.0) {
                    throttle = old_throttle + 5.0;
                }
            }

            else {
                throttle = 35.0;

                if (threat)
                    throttle = 100.0;

                brakes = accumulator.brake;

                if (brakes > 0.1)
                    throttle = 0.0;
            }
        }

        else { // wingman
            Ship* lead = elem ? elem->GetShip(1) : nullptr;
            const double zone = ship->Radius() * 3.0;

            if (lead)
                desired = (lead->Velocity() | lead->Heading());

            // try to prevent porpoising
            if (FMath::Abs(slot_dist) < distance / 4.0) {
                throttle = old_throttle;
            }

            else if (slot_dist > zone * 2.0) {
                throttle = 100.0;

                if (objective.Z > 10e3 && ship_speed < desired && ship->GetFuelLevel() > 25.0)
                    augmenter = true;
            }

            else if (slot_dist > zone) {
                throttle = lead ? (lead->Throttle() + 10.0) : old_throttle;
            }

            else if (slot_dist < -zone * 2.0) {
                throttle = old_throttle - 10.0;
                brakes = 1.0;
            }

            else if (slot_dist < -zone) {
                throttle = old_throttle;
                brakes = 0.5;
            }

            else if (lead) {
                const double lv = lead->Velocity().Size();
                const double sv = ship_speed;
                const double dv = lv - sv;
                double dt = 0.0;

                if (dv > 0.0)       dt = dv * 1e-5 * frame_time;
                else if (dv < 0.0)  dt = dv * 1e-2 * frame_time;

                throttle = old_throttle + dt;
            }

            else {
                throttle = old_throttle;
            }
        }
    }

    // TARGET/WARD/NAVPOINT SEEKING
    else {
        throttle = old_throttle;

        if (target) {
            desired = 1250.0;

            if (ai_level < 1) {
                throttle = 70.0;
            }

            else if (ship->IsAirborne()) {
                throttle = 100.0;

                if (!threat_missile && FMath::Abs(objective.Z) > 6e3 && ship->GetFuelLevel() > 25.0)
                    augmenter = true;
            }

            else {
                throttle = 100.0;

                if (objective.Z > 20e3 && ship_speed < desired && ship->GetFuelLevel() > 35.0)
                    augmenter = true;

                else if (objective.Z > 0.0 && objective.Z < 10e3)
                    throttle = 50.0;
            }
        }

        else if (ward) {
            const double d = (ship->Location() - ward->Location()).Size();

            if (d > 5000.0) {
                throttle = (ai_level < 1) ? 50.0 : 80.0;
            }
            else {
                const double speed = ward->Velocity().Size();

                if (speed > 0.0) {
                    if (ship_speed > speed) {
                        throttle = old_throttle - 5.0;
                        brakes = 0.25;
                    }
                    else if (ship_speed < speed - 10.0) {
                        throttle = old_throttle + 1.0;
                    }
                }
            }
        }

        else if (navpt) {
            desired = navpt->Speed();

            if (hold) {
                // go into a slow orbit if airborne:
                if (ship->IsAirborne() && ship->Class() < CLASSIFICATION::LCA) {
                    throttle = 25.0;
                    brakes = 0.0;
                }
                else {
                    throttle = 0.0;
                    brakes = 1.0;
                }
            }

            else if (desired > 0.0) {
                if (ship_speed > desired) {
                    throttle = old_throttle - 5.0;
                    brakes = 0.25;
                }

                else if (ship_speed < 0.85 * desired) {
                    throttle = 100.0;

                    if ((ship->IsAirborne() || ship_speed < 0.35 * desired) && ship->GetFuelLevel() > 30.0)
                        augmenter = true;
                }

                else if (ship_speed < desired - 10.0) {
                    throttle = old_throttle + 1.0;
                }

                else if (Ship::GetFlightModel() > 0) {
                    throttle = old_throttle;
                }
            }
        }

        else {
            throttle = 0.0;
            brakes = 1.0;
        }
    }

    // clamp / floor behavior (preserve original semantics)
    if (ship->IsAirborne() && throttle < 20.0 && ship->Class() < CLASSIFICATION::LCA)
        throttle = 20.0;
    else if (ship->Design()->auto_roll > 1 && throttle < 5.0)
        throttle = 5.0;
    else if (throttle < 0.0)
        throttle = 0.0;

    old_throttle = throttle;

    ship->SetThrottle((int)throttle);
    ship->SetAugmenter(augmenter);

    if (accumulator.stop && ship->GetFLCS() != 0)
        ship->GetFLCS()->FullStop();

    else if (ship_speed > 1.0 && brakes > 0.0)
        ship->SetTransY(-brakes * ship->Design()->trans_y);

    else if (throttle > 10.0 && (ship->GetEMCON() < 2 || ship->GetFuelLevel() < 10.0))
        ship->SetTransY(ship->Design()->trans_y);
}

Steer
FighterAI::AvoidTerrain()
{
    Steer avoid;

    terrain_warning = false;

    if (!ship || !ship->GetRegion() || !ship->GetRegion()->IsActive() ||
        (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH)) {
        return avoid;
    }

    if (ship->IsAirborne() && ship->GetFlightPhase() == Ship::ACTIVE) {
        // too high?
        if (ship->AltitudeMSL() > 25e3) {
            if (!navpt || (navpt->Region() == ship->GetRegion() && navpt->Location().Z < 27e3)) {
                terrain_warning = true;
                ship->SetDirectorInfo("Too High");

                // where will we be?
                const FVector SelfPt = ship->Location() + ship->Velocity() + FVector(0.0, 0.0, -15e3);

                // transform into camera coords:
                const FVector Obj = Transform(SelfPt);

                // head down!
                avoid = Seek(Obj);
            }
        }

        // too low?
        else if (ship->AltitudeAGL() < 2500) {
            terrain_warning = true;
            ship->SetDirectorInfo("Too Low");

            // way too low?
            if (ship->AltitudeAGL() < 1500) {
                ship->SetDirectorInfo(Game::GetText("Way Too Low!"));
                target = nullptr;
                drop_time = 5.0;
            }

            // where will we be?
            const FVector SelfPt = ship->Location() + ship->Velocity() + FVector(0.0, 0.0, 10e3);

            // transform into camera coords:
            const FVector Obj = Transform(SelfPt);

            // pull up!
            avoid = Seek(Obj);
        }
    }

    return avoid;
}

// +--------------------------------------------------------------------+

Steer
FighterAI::SeekTarget()
{
    if (ship->GetFlightPhase() < Ship::ACTIVE)
        return Seek(objective);

    Ship* ward = ship->GetWard();

    if ((!target && !ward && !navpt && !farcaster && !patrol && !inbound && !rtb_code) ||
        ship->MissionClock() < 10000) {
        if (element_index > 1) {
            // break formation if threatened:
            if (threat_missile)
                return Steer();

            else if (threat && !form_up)
                return Steer();

            // otherwise, keep in formation:
            return SeekFormationSlot();
        }

        return Steer();
    }

    if (patrol) {
        Steer result = Seek(objective);
        ship->SetDirectorInfo(Game::GetText("ai.seek-patrol-point"));

        if (distance < 10 * self->Radius()) {
            patrol = 0;
            result.brake = 1;
            result.stop = 1;
        }

        return result;
    }

    if (inbound) {
        Steer result = Seek(objective);

        if (over_threshold && objective.Z < 0) {
            result = Steer();
            result.brake = 1;
            result.stop = 1;
        }
        else {
            ship->SetDirectorInfo(Game::GetText("ai.seek-inbound"));

            // approach legs:
            if (inbound->Approach() > 0) {
                if (distance < 20 * self->Radius())
                    inbound->SetApproach(inbound->Approach() - 1);
            }

            // marshall point and finals:
            else {
                if (inbound->Cleared() && distance < 10 * self->Radius()) {
                    if (!inbound->Final()) {
                        time_to_dock = TIME_TO_DOCK;

                        FlightDeck* deck = inbound->GetDeck();
                        if (deck) {
                            const double TotalDist = (deck->EndPoint() - deck->StartPoint()).Size();
                            const double CurrentDist = (deck->EndPoint() - ship->Location()).Size();

                            if (TotalDist > 1e-3)
                                time_to_dock *= (CurrentDist / TotalDist);
                        }

                        RadioTraffic::SendQuickMessage(ship, RadioMessageAction::CALL_FINALS);
                    }

                    inbound->SetFinal(true);
                    ship->LowerGear();
                    result.brake = 1;
                    result.stop = 1;
                }

                else if (!inbound->Cleared() && distance < 2000) {
                    ship->SetDirectorInfo(Game::GetText("ai.hold-final"));
                    result = Steer();
                    result.brake = 1;
                    result.stop = 1;
                }
            }
        }

        return result;
    }

    else if (rtb_code) {
        return Seek(objective);
    }

    SimObject* tgt = target;

    if (ward && !tgt)
        tgt = ward;

    if (tgt && too_close == tgt->Identity()) {
        drop_time = 4.0;
        return Steer();
    }

    else if (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH) {
        ship->SetDirectorInfo("Launch");
        return Seek(objective);
    }

    else if (farcaster) {
        // wingmen should:
        if (element_index > 1)
            return SeekFormationSlot();

        ship->SetDirectorInfo("Seek Farcaster");
        return Seek(objective);
    }

    else if (drop_time > 0.0) {
        return Steer();
    }

    if (tgt) {
        const double basis = self->Radius() + tgt->Radius();
        const double gap = distance - basis;

        // target behind:
        if (objective.Z < 0) {
            // leave some room for an attack run:
            if (gap < 8000) {
                Steer s;

                s.pitch = -0.1;
                if (objective.X > 0) s.yaw = 0.1;
                else                 s.yaw = -0.1;

                return s;
            }

            // start the attack run:
            return Seek(objective);
        }

        // target in front:
        else {
            if (tgt->Type() == SimObject::SIM_SHIP) {
                Ship* tgt_ship = (Ship*)tgt;

                // capital target strike:
                if (tgt_ship->IsStatic()) {
                    if (gap < 2500)
                        return Flee(objective);
                }

                else if (tgt_ship->IsStarship()) {
                    if (gap < 1000)
                        return Flee(objective);

                    else if (ship->GetFlightModel() == Ship::FM_STANDARD && gap < 20e3)
                        go_manual = true;
                }
            }

            // fighter melee:
            if ((tgt->Velocity() | ship->Velocity()) < 0) {
                // head-to-head pass:
                if (gap < 1250)
                    return Flee(objective);
            }

            else if (gap < 250) {
                return Steer();
            }

            ship->SetDirectorInfo("Seek Target");
            return Seek(objective);
        }
    }

    if (navpt) {
        ship->SetDirectorInfo("Seek Navpoint");
    }

    return Seek(objective);
}

// +--------------------------------------------------------------------+

Steer
FighterAI::SeekFormationSlot()
{
    Steer s;

    // advance memory pipeline:
    az[2] = az[1]; az[1] = az[0];
    el[2] = el[1]; el[1] = el[0];

    SimElement* elem = ship->GetElement();
    Ship* lead = elem ? elem->GetShip(1) : nullptr;

    if (lead) {
        SimRegion* self_rgn = ship->GetRegion();
        SimRegion* lead_rgn = lead->GetRegion();

        if (self_rgn != lead_rgn) {
            QuantumDrive* qdrive = ship->GetQuantumDrive();
            const bool use_farcaster = !qdrive || !qdrive->IsPowerOn() || qdrive->GetStatus() < SYSTEM_STATUS::DEGRADED;

            if (use_farcaster) {
                FindObjectiveFarcaster(self_rgn, lead_rgn);
            }
            else if (qdrive) {
                if (qdrive->ActiveState() == QuantumDrive::ACTIVE_READY) {
                    qdrive->SetDestination(lead_rgn, lead->Location());
                    qdrive->Engage();
                }
            }
        }
    }

    // do station keeping?
    if (lead && distance < ship->Radius() * 10 && lead->Velocity().Size() < 50) {
        distance = -1;
        return s;
    }

    // approach
    if (objective.Z > ship->Radius() * -4) {
        az[0] = atan2(FMath::Abs(objective.X), objective.Z) * 50.0;
        el[0] = atan2(FMath::Abs(objective.Y), objective.Z) * 50.0;

        if (objective.X < 0) az[0] = -az[0];
        if (objective.Y > 0) el[0] = -el[0];

        s.yaw = az[0] - seek_damp * (az[1] + az[2] * 0.5);
        s.pitch = el[0] - seek_damp * (el[1] + el[2] * 0.5);
    }

    // reverse
    else {
        s.yaw = (objective.X > 0) ? 1.0 : -1.0;
        s.pitch = -objective.Y * 0.5;
    }

    seeking = 1;
    ship->SetDirectorInfo(Game::GetText("ai.seek-formation"));

    return s;
}

// +--------------------------------------------------------------------+

Steer
FighterAI::Seek(const Point& point)
{
    Steer s;

    // advance memory pipeline:
    az[2] = az[1]; az[1] = az[0];
    el[2] = el[1]; el[1] = el[0];

    // approach
    if (point.Z > 0.0) {
        az[0] = atan2(FMath::Abs(point.X), point.Z) * seek_gain;
        el[0] = atan2(FMath::Abs(point.Y), point.Z) * seek_gain;

        if (point.X < 0) az[0] = -az[0];
        if (point.Y > 0) el[0] = -el[0];

        s.yaw = az[0] - seek_damp * (az[1] + az[2] * 0.5);
        s.pitch = el[0] - seek_damp * (el[1] + el[2] * 0.5);

        // pull up:
        if (ship->IsAirborne() && point.Y > 5e3)
            s.pitch = -1.0;
    }

    // reverse
    else {
        if (ship->IsAirborne()) {
            // pull up:
            if (point.Y > 5e3) {
                s.pitch = -1.0;
            }

            // head down:
            else if (point.Y < -5e3) {
                s.pitch = 1.0;
            }

            // level turn:
            else {
                s.yaw = (point.X > 0) ? 1.0 : -1.0;
                s.brake = 0.5;
            }
        }

        else {
            s.yaw = (point.X > 0) ? 1.0 : -1.0;
        }
    }

    seeking = 1;

    return s;
}

// +--------------------------------------------------------------------+

Steer
FighterAI::EvadeThreat()
{
    // MISSILE THREAT REACTION:
    if (threat_missile) {
        evading = true;
        SetTarget(nullptr);
        drop_time = 3.0 * (3 - ai_level);

        // dropped a decoy for this missile yet?
        if (decoy_missile != threat_missile) {
            ship->FireDecoy();
            decoy_missile = threat_missile;
        }

        // beam the missile:
        ship->SetDirectorInfo(Game::GetText("ai.evade-missile"));

        FVector BeamLine = FVector::CrossProduct(threat_missile->Velocity(), FVector(0.0, 1.0, 0.0));
        BeamLine.Normalize();
        BeamLine *= 1e6;

        FVector EvadeW1 = threat_missile->Location() + BeamLine;
        FVector EvadeW2 = threat_missile->Location() - BeamLine;

        const double D1 = (EvadeW1 - ship->Location()).Size();
        const double D2 = (EvadeW2 - ship->Location()).Size();

        const FVector EvadeP = (D1 > D2) ? Transform(EvadeW1) : Transform(EvadeW2);
        return Seek(EvadeP);
    }

    // GENERAL THREAT EVASION:
    if (threat && !form_up) {
        double threat_range = 20e3;

        Ship* threat_ship = (Ship*)threat;
        const double threat_dist = (threat->Location() - ship->Location()).Size();

        if (threat_ship->IsStarship()) {
            threat_range = CalcDefensePerimeter(threat_ship);
        }

        if (threat_dist <= threat_range) {
            ship->SetDirectorInfo(Game::GetText("ai.evade-threat"));

            if (ship->IsAirborne()) {
                evading = true;

                FVector BeamLine = FVector::CrossProduct(threat->Velocity(), FVector(0.0, 1.0, 0.0));
                BeamLine.Normalize();
                BeamLine *= threat_range;

                const FVector EvadeW = threat->Location() + BeamLine;
                const FVector EvadeP = Transform(EvadeW);

                return Seek(EvadeP);
            }

            else if (threat_ship->IsStarship()) {
                evading = true;

                if (target == threat_ship && threat_dist < threat_range / 4.0) {
                    SetTarget(nullptr);
                    drop_time = 5.0;
                }

                if (!target) {
                    ship->SetDirectorInfo(Game::GetText("ai.evade-starship"));

                    // flee for three seconds:
                    if ((ship->MissionClock() & 3) != 3) {
                        return Flee(Transform(threat->Location()));
                    }

                    // jink for one second:
                    else {
                        if (Game::GameTime() - jink_time > 1500) {
                            jink_time = Game::GameTime();
                            jink = FVector(
                                FMath::RandRange(-16384, 16384),
                                FMath::RandRange(-16384, 16384),
                                FMath::RandRange(-16384, 16384)
                            ) * 15e3;
                        }

                        const FVector EvadeW = ship->Location() + jink;
                        const FVector EvadeP = Transform(EvadeW);

                        return Seek(EvadeP);
                    }
                }

                else {
                    ship->SetDirectorInfo(Game::GetText("ai.evade-and-seek"));

                    // seek for three seconds:
                    if ((ship->MissionClock() & 3) < 3) {
                        return Steer(); // no evasion
                    }

                    // jink for one second:
                    else {
                        if (Game::GameTime() - jink_time > 1000) {
                            jink_time = Game::GameTime();
                            jink = FVector(
                                FMath::RandRange(-16384, 16384),
                                FMath::RandRange(-16384, 16384),
                                FMath::RandRange(-16384, 16384)
                            );
                        }

                        const FVector EvadeW = target->Location() + jink;
                        const FVector EvadeP = Transform(EvadeW);

                        return Seek(EvadeP);
                    }
                }
            }

            else {
                evading = true;

                if (target != nullptr) {
                    if (target == threat) {
                        if (target->Type() == SimObject::SIM_SHIP) {
                            Ship* tgt_ship = (Ship*)target;
                            if (tgt_ship->GetTrigger(0)) {
                                SetTarget(nullptr);
                                drop_time = 3.0;
                            }
                        }
                    }
                    else if (target && threat_dist < threat_range / 2.0) {
                        SetTarget(nullptr);
                        drop_time = 3.0;
                    }
                }

                if (target)
                    ship->SetDirectorInfo(Game::GetText("ai.evade-and-seek"));
                else
                    ship->SetDirectorInfo(Game::GetText("ai.random-evade"));

                // beam the threat:
                FVector BeamLine = FVector::CrossProduct(threat->Velocity(), FVector(0.0, 1.0, 0.0));
                BeamLine.Normalize();
                BeamLine *= 1e6;

                FVector EvadeW1 = threat->Location() + BeamLine;
                FVector EvadeW2 = threat->Location() - BeamLine;

                const double D1 = (EvadeW1 - ship->Location()).Size();
                const double D2 = (EvadeW2 - ship->Location()).Size();

                FVector EvadeP = (D1 > D2) ? Transform(EvadeW1) : Transform(EvadeW2);

                if (!target) {
                    const uint32 JinkRate = (uint32)(400 + 200 * (3 - ai_level));

                    if ((uint32)(Game::GameTime() - jink_time) > JinkRate) {
                        jink_time = Game::GameTime();
                        jink = FVector(
                            FMath::RandRange(-16384, 16384),
                            FMath::RandRange(-16384, 16384),
                            FMath::RandRange(-16384, 16384)
                        ) * 2000.0;
                    }

                    EvadeP += jink;
                }

                Steer steer = Seek(EvadeP);

                if (target)
                    return steer / 4;

                return steer;
            }
        }
    }

    return Steer();
}

// +--------------------------------------------------------------------+

void
FighterAI::FireControl()
{
    // if nothing to shoot at, forget it:
    if (!target || target->Integrity() < 1)
        return;

    // if the objective is a navpt or landing bay (not a target), then don't shoot!
    if (inbound || farcaster || (navpt && navpt->GetAction() < INSTRUCTION_ACTION::DEFEND))
        return;

    // object behind us, or too close:
    if (objective.Z < 0 || distance < 4 * self->Radius())
        return;

    // compute the firing cone:
    const double cross_section = 2.0 * target->Radius() / distance;
    double gun_basket = cross_section * 2.0;

    Weapon* primary = ship->GetPrimary();
    Weapon* secondary = ship->GetSecondary();
    const WeaponDesign* dsgn_primary = nullptr;
    const WeaponDesign* dsgn_secondary = nullptr;
    bool use_primary = true;
    Ship* tgt_ship = nullptr;

    if (target->Type() == SimObject::SIM_SHIP) {
        tgt_ship = (Ship*)target;

        if (tgt_ship->InTransition())
            return;
    }

    if (primary) {
        dsgn_primary = primary->Design();

        if (dsgn_primary &&
            dsgn_primary->aim_az_max > 5 * DEGREES &&
            distance > dsgn_primary->max_range / 2.0) {
            gun_basket = cross_section * 4.0;
        }

        gun_basket *= (3 - ai_level);

        if (tgt_ship) {
            if (!primary->CanTarget((uint32)tgt_ship->Class()))
                use_primary = false;

            /*** XXX NEED TO SUBTARGET SYSTEMS IF TARGET IS STARSHIP...
            else if (tgt_ship->ShieldStrength() > 10)
                use_primary = false;
            ***/
        }

        if (use_primary && dsgn_primary) {
            // is target in the basket?
            const double dx = FMath::Abs(objective.X / distance);
            const double dy = FMath::Abs(objective.Y / distance);

            if (primary->GetFiringOrders() == WeaponsOrders::MANUAL &&
                dx < gun_basket && dy < gun_basket &&
                distance > dsgn_primary->min_range &&
                distance < dsgn_primary->max_range &&
                !primary->IsBlockedFriendly()) {
                ship->FirePrimary();
            }
        }
    }

    if (secondary && secondary->GetFiringOrders() == WeaponsOrders::MANUAL) {
        dsgn_secondary = secondary->Design();

        if (missile_time <= 0.0 && secondary->Ammo() && !secondary->IsBlockedFriendly()) {
            if (secondary->Locked() || (dsgn_secondary && !dsgn_secondary->self_aiming)) {
                // is target in basket?
                FVector tgt = AimTransform(target->Location());
                const double tgt_range = tgt.Normalize();

                const int factor = 2 - ai_level;
                double s_range = 0.5 + 0.2 * factor;
                double s_basket = 0.3 + 0.2 * factor;
                double extra_time = 10.0 * factor * factor + 5.0;

                if (dsgn_secondary && !dsgn_secondary->self_aiming)
                    s_basket *= 0.33;

                if (tgt_ship) {
                    if (tgt_ship->Class() == CLASSIFICATION::MINE) {
                        extra_time = 10.0;
                        s_range = 0.75;
                    }

                    else if (!tgt_ship->IsDropship()) {
                        extra_time = 0.5 * factor + 0.5;
                        s_range = 0.9;
                    }
                }

                // is target in decent range?
                if (dsgn_secondary &&
                    tgt_range < secondary->Design()->max_range * s_range) {

                    const double dx = FMath::Abs(tgt.X);
                    const double dy = FMath::Abs(tgt.Y);

                    if (dx < s_basket && dy < s_basket && tgt.Z > 0.0) {
                        if (ship->FireSecondary()) {
                            missile_time = secondary->Design()->salvo_delay + extra_time;

                            if (Game::GameTime() - last_call_time > 6000) {
                                // call fox:
                                RadioMessageAction call = RadioMessageAction::FOX_3;                 // A2A

                                if (secondary->CanTarget((int)CLASSIFICATION::GROUND_UNITS))   // AGM
                                    call = RadioMessageAction::FOX_1;

                                else if (secondary->CanTarget((int)CLASSIFICATION::DESTROYER)) // ASM
                                    call = RadioMessageAction::FOX_2;

                                RadioTraffic::SendQuickMessage(ship, call);
                                last_call_time = Game::GameTime();
                            }
                        }
                    }
                }
            }
        }
    }
}

// +--------------------------------------------------------------------+

double
FighterAI::CalcDefensePerimeter(Ship* starship)
{
    double perimeter = 15e3;

    if (starship) {
        ListIter<WeaponGroup> g_iter = starship->GetWeapons();
        while (++g_iter) {
            WeaponGroup* group = g_iter.value();

            ListIter<Weapon> w_iter = group->GetWeapons();
            while (++w_iter) {
                Weapon* weapon = w_iter.value();

                if (weapon->Ammo() &&
                    weapon->GetTarget() == ship &&
                    !weapon->IsBlockedFriendly()) {

                    const double range = weapon->Design()->max_range * 1.2;
                    if (range > perimeter)
                        perimeter = range;
                }
            }
        }
    }

    return perimeter;
}
