/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         NavAI.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2007. All Rights Reserved.

    OVERVIEW
    ========
    Automatic Navigator Artificial Intelligence class
*/

#include "NavAI.h"

#include "TacticalAI.h"
#include "Instruction.h"
#include "NavSystem.h"
#include "QuantumDrive.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipManager.h"
#include "Drive.h"
#include "Farcaster.h"
#include "Shield.h"
#include "Sim.h"
#include "StarSystem.h"
#include "KeyMap.h"
#include "HUDView.h"
#include "HUDSounds.h"
#include "GameStructs.h"

#include "Game.h"

#include "CoreMinimal.h"              // UE_LOG
#include "Math/UnrealMathUtility.h"   // FMath
#include "Math/Vector.h"              // FVector

// +----------------------------------------------------------------------+

NavAI::NavAI(Ship* s)
    : ShipAI(s),
    complete(false),
    drop_state(0),
    quantum_state(0),
    terrain_warning(0),
    brakes(0),
    farcaster(0)
{
    seek_gain = 20;
    seek_damp = 0.55;

    delete tactical;
    tactical = 0;
}

// +--------------------------------------------------------------------+

NavAI::~NavAI()
{
}

// +--------------------------------------------------------------------+

void
NavAI::ExecFrame(double s)
{
    if (!ship) return;

    seconds = s;

    ship->SetDirectorInfo(" ");

    if (ship->GetFlightPhase() == Ship::TAKEOFF)
        takeoff = true;

    else if (takeoff && ship->MissionClock() > 10000)
        takeoff = false;

    FindObjective();
    Navigator();

    // watch for disconnect:
    if (ShipManager::Toggled(KEY_AUTO_NAV)) {
        NavSystem* navsys = ship->GetNavSystem();
        if (navsys) {
            HUDView::GetInstance()->SetHUDMode(EHUDMode::Tactical);
            navsys->DisengageAutoNav();

            Sim* sim = Sim::GetSim();
            if (sim) {
                ship->SetControls(sim->GetControls());
                return;
            }
        }
    }

    static double time_til_change = 0.0;

    if (time_til_change < 0.001) {
        if (ship->GetShield()) {
            Shield* shield = ship->GetShield();
            double  level = shield->GetPowerLevel();

            if (ShipManager::KeyDown(KEY_SHIELDS_FULL)) {
                HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
                shield->SetPowerLevel(100);
                time_til_change = 0.5f;
            }

            else if (ShipManager::KeyDown(KEY_SHIELDS_ZERO)) {
                HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
                shield->SetPowerLevel(0);
                time_til_change = 0.5f;
            }

            else if (ShipManager::KeyDown(KEY_SHIELDS_UP)) {
                if (level < 25)      level = 25;
                else if (level < 50) level = 50;
                else if (level < 75) level = 75;
                else                 level = 100;

                HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
                shield->SetPowerLevel(level);
                time_til_change = 0.5f;
            }

            else if (ShipManager::KeyDown(KEY_SHIELDS_DOWN)) {
                if (level > 75)      level = 75;
                else if (level > 50) level = 50;
                else if (level > 25) level = 25;
                else                 level = 0;

                HUDSounds::PlaySound(HUDSounds::SND_SHIELD_LEVEL);
                shield->SetPowerLevel(level);
                time_til_change = 0.5f;
            }
        }
    }
    else {
        time_til_change -= seconds;
    }

    if (ShipManager::Toggled(KEY_DECOY))
        ship->FireDecoy();

    if (ShipManager::Toggled(KEY_LAUNCH_PROBE))
        ship->LaunchProbe();

    if (ShipManager::Toggled(KEY_GEAR_TOGGLE))
        ship->ToggleGear();

    if (ShipManager::Toggled(KEY_NAVLIGHT_TOGGLE))
        ship->ToggleNavlights();

    if (drop_state < 0) {
        ship->DropOrbit();
        return;
    }

    if (drop_state > 0) {
        ship->MakeOrbit();
        return;
    }
}

// +--------------------------------------------------------------------+

void NavAI::FindObjective()
{
    navpt = nullptr;
    distance = 0.0;
    complete = false;

    // runway takeoff:
    if (takeoff) {
        obj_w = ship->Location() + ship->Heading() * 10e3;
        obj_w.Y = ship->Location().Y + 2e3;

        objective = Transform(obj_w);
        ship->SetDirectorInfo(Game::GetText("ai.takeoff"));
        return;
    }

    // PART I: Find next NavPoint:
    if (ship->GetNavSystem())
        navpt = ship->GetNextNavPoint();

    complete = (navpt == nullptr);
    if (complete)
        return;

    // PART II: Compute Objective from NavPoint:
    const FVector npt = navpt->Location();

    SimRegion* self_rgn = ship->GetRegion();
    SimRegion* nav_rgn = navpt->Region();

    if (self_rgn && !nav_rgn) {
        nav_rgn = self_rgn;
        navpt->SetRegion(nav_rgn);
    }

    // Guard against null regions (UE safety; avoids null deref below)
    if (!self_rgn || !nav_rgn) {
        obj_w = npt;
        distance = FVector::Dist(obj_w, ship->Location());
        objective = Transform(obj_w);

        if (!ship->IsStarship())
            objective.Normalize();

        return;
    }

    if (self_rgn == nav_rgn) {
        if (farcaster) {
            if (farcaster->GetShip() && farcaster->GetShip()->GetRegion() != self_rgn) {
                const Ship* DestShip = farcaster->GetDest();
                farcaster = (DestShip ? DestShip->GetFarcaster() : nullptr);
            }

            if (farcaster)
                obj_w = farcaster->EndPoint();
            else
                obj_w = npt;
        }
        else {
            // UE: FVector has no OtherHand(). Use the location directly.
            // If you still need axis remapping, do it explicitly here.
            obj_w = npt;
        }

        // distance from self to navpt:
        distance = FVector::Dist(obj_w, ship->Location());

        // transform into camera coords:
        objective = Transform(obj_w);

        if (!ship->IsStarship())
            objective.Normalize();

        if (farcaster && distance < 1000.0)
            farcaster = nullptr;
    }

    // PART III: Deal with orbital transitions:
    else if (ship->IsDropship()) {

        // Extra null safety:
        const OrbitalRegion* SelfOrb = self_rgn->GetOrbitalRegion();
        const OrbitalRegion* NavOrb = nav_rgn->GetOrbitalRegion();

        if (SelfOrb && NavOrb && NavOrb->Primary() == SelfOrb->Primary()) {

            const FVector npt_rel = nav_rgn->GetLocation() - self_rgn->GetLocation();
            obj_w = npt_rel; // UE: no OtherHand()

            distance = FVector::Dist(obj_w, ship->Location());
            objective = Transform(obj_w);

            if (nav_rgn->IsAirSpace()) {
                drop_state = -1;
            }
            else if (nav_rgn->IsOrbital()) {
                drop_state = 1;
            }
        }

        // PART IIIa: Deal with farcaster jumps:
        else if (nav_rgn->IsOrbital() && self_rgn->IsOrbital()) {

            ListIter<Ship> s = self_rgn->GetShips();
            while (++s && !farcaster) {
                if (s->GetFarcaster()) {
                    const Ship* Dest = s->GetFarcaster()->GetDest();
                    if (Dest && Dest->GetRegion() == nav_rgn) {
                        farcaster = s->GetFarcaster();
                    }
                }
            }

            if (farcaster) {
                const FVector apt = farcaster->ApproachPoint(0);
                const FVector fp0 = farcaster->StartPoint();

                const double r1 = (ship->Location() - fp0).Length();

                if (r1 > 50e3) {
                    obj_w = apt;
                    distance = r1;
                    objective = Transform(obj_w);
                }
                else {
                    const double r2 = (ship->Location() - apt).Length();
                    const double r3 = (fp0 - apt).Length();

                    if (r1 + r2 < 1.2 * r3) {
                        obj_w = fp0;
                        distance = r1;
                        objective = Transform(obj_w);
                    }
                    else {
                        obj_w = apt;
                        distance = r2;
                        objective = Transform(obj_w);
                    }
                }
            }
        }
    }

    // PART IV: Deal with quantum jumps:
    else if (ship->IsStarship()) {
        quantum_state = 1;

        // UE: FVector supports + and -=. navpt->Location() is already a world-space point in most ports.
        // Original code mixed region offsets; keep the intent but remove OtherHand().
        FVector npt_rel = nav_rgn->GetLocation() + navpt->Location();
        npt_rel -= self_rgn->GetLocation();

        obj_w = npt_rel;

        distance = FVector::Dist(obj_w, ship->Location());
        objective = Transform(obj_w);
    }
}


// +--------------------------------------------------------------------+

void
NavAI::Navigator()
{
    accumulator.Clear();
    magnitude = 0;
    brakes = 0;
    hold = false;

    if (navpt) {
        if (navpt->GetStatus() == INSTRUCTION_STATUS::COMPLETE && navpt->HoldTime() > 0) {
            ship->SetDirectorInfo("AutoHold");
            hold = true;
        }
        else {
            ship->SetDirectorInfo("AutoNav");
        }
    }
    else {
        ship->SetDirectorInfo("AutoStop");
    }

    Accumulate(AvoidTerrain());
    Accumulate(AvoidCollision());

    if (!hold)
        accumulator = SeekTarget();

    HelmControl();
    ThrottleControl();
}

// +--------------------------------------------------------------------+

void
NavAI::HelmControl()
{
    // ----------------------------------------------------------
    // STARSHIP HELM MODE
    // ----------------------------------------------------------

    if (ship->IsStarship()) {
        ship->SetFLCSMode(Ship::FLCS_HELM);
        ship->SetHelmHeading(accumulator.yaw);

        if (accumulator.pitch > 45 * DEGREES)
            ship->SetHelmPitch(45 * DEGREES);

        else if (accumulator.pitch < -45 * DEGREES)
            ship->SetHelmPitch(-45 * DEGREES);

        else
            ship->SetHelmPitch(accumulator.pitch);
    }

    // ----------------------------------------------------------
    // FIGHTER FLCS AUTO MODE
    // ----------------------------------------------------------

    else {
        ship->SetFLCSMode(Ship::FLCS_AUTO);

        // are we being asked to flee?
        if (fabs(accumulator.yaw) == 1.0 && accumulator.pitch == 0.0) {
            accumulator.pitch = -0.7f;
            accumulator.yaw *= 0.25f;
        }

        self->ApplyRoll((float)(accumulator.yaw * -0.4));
        self->ApplyYaw((float)(accumulator.yaw * 0.2));

        if (fabs(accumulator.yaw) > 0.5 && fabs(accumulator.pitch) < 0.1)
            accumulator.pitch -= 0.1f;

        if (accumulator.pitch != 0)
            self->ApplyPitch((float)accumulator.pitch);

        // if not turning, roll to orient with world coords:
        if (fabs(accumulator.yaw) < 0.1) {
            FVector vrt = ((Camera*)&(self->Cam()))->vrt();
            double deflection = vrt.Y;
            if (deflection != 0) {
                double theta = asin(deflection / vrt.Length());
                self->ApplyRoll(-theta);
            }
        }

        if (!ship->IsAirborne() || ship->AltitudeAGL() > 100)
            ship->RaiseGear();
    }

    ship->SetTransX(0);
    ship->SetTransY(0);
    ship->SetTransZ(0);
    ship->ExecFLCSFrame();
}

// +--------------------------------------------------------------------+

void NavAI::ThrottleControl()
{
    if (!ship) {
        throttle = 0;
        old_throttle = 0;
        brakes = 0;
        return;
    }

    // UE: FVector dot product for forward speed along heading:
    const double ShipSpeed = FVector::DotProduct(ship->Velocity(), ship->Heading());

    bool bAugmenter = false;

    // Defaults each tick (prevents stale values from previous frames)
    brakes = 0.0;
    throttle = 0.0;

    if (hold) {
        throttle = 0.0;
        brakes = 1.0;
    }
    else if (navpt) {
        double DesiredSpeed = navpt->Speed();
        if (DesiredSpeed < 10.0)
            DesiredSpeed = 250.0;

        if (Ship::GetFlightModel() > 0) {
            // Smooth throttle adjustments:
            if (ShipSpeed > DesiredSpeed + 10.0)
                throttle = old_throttle - 0.25;
            else if (ShipSpeed < DesiredSpeed - 10.0)
                throttle = old_throttle + 0.25;
            else
                throttle = old_throttle;
        }
        else {
            // Arcade-ish model: brake if fast, otherwise set a moderate throttle:
            if (ShipSpeed > DesiredSpeed + 5.0) {
                brakes = 0.25;
            }
            else if (ShipSpeed < DesiredSpeed - 5.0) {
                throttle = 50.0;
            }
        }
    }
    else {
        throttle = 0.0;
        brakes = 0.25;
    }

    // Ensure fighter-class airborne craft don't stall:
    if (ship->IsAirborne() && ship->Class() < CLASSIFICATION::LCA) {
        if (ShipSpeed < 250.0) {
            throttle = 100.0;
            brakes = 0.0;

            if (ShipSpeed < 200.0)
                bAugmenter = true;
        }
        else if (throttle < 20.0) {
            throttle = 20.0;
        }
    }

    // Optional safety clamp (keeps behavior sane if upstream code overshoots)
    if (throttle < 0.0)   throttle = 0.0;
    if (throttle > 100.0) throttle = 100.0;
    if (brakes < 0.0)   brakes = 0.0;
    if (brakes > 1.0)   brakes = 1.0;

    old_throttle = throttle;

    ship->SetThrottle(throttle);
    ship->SetAugmenter(bAugmenter);

    // Braking thrust: negative Y translation (same intent as original)
    if (ShipSpeed > 1.0 && brakes > 0.0) {
        const double MaxTransY = ship->Design() ? ship->Design()->trans_y : 0.0;
        ship->SetTransY(-brakes * MaxTransY);
    }
}

// +--------------------------------------------------------------------+

Steer
NavAI::SeekTarget()
{
    if (!ship)
        return Steer();

    if (takeoff)
        return Seek(objective);

    if (navpt) {
        if (quantum_state == 1) {
            QuantumDrive* q = ship->GetQuantumDrive();

            if (q) {
                if (q->ActiveState() == QuantumDrive::ACTIVE_READY) {
                    q->SetDestination(navpt->Region(), navpt->Location());
                    q->Engage();
                }

                else if (q->ActiveState() == QuantumDrive::ACTIVE_POSTWARP) {
                    quantum_state = 0;
                }
            }
        }

        if (distance < 2 * self->Radius()) {
            ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::COMPLETE);
            return Steer();
        }
        else {
            return Seek(objective);
        }
    }

    return Steer();
}

// +--------------------------------------------------------------------+

void
NavAI::Disengage()
{
    throttle = 0;
}

// +--------------------------------------------------------------------+

FVector
NavAI::Transform(const FVector& Point)
{
    if (ship && ship->IsStarship())
        return Point - self->Location();

    return SteerAI::Transform(Point);
}

Steer
NavAI::Seek(const FVector& Point)
{
    // if ship is starship, the point is in relative world coordinates
    //   X: distance east(-)  / west(+)
    //   Y: altitude down(-)  / up(+)
    //   Z: distance north(-) / south(+)

    if (ship && ship->IsStarship()) {
        Steer Result;

        Result.yaw = atan2(Point.X, Point.Z) + PI;

        double Adjacent = sqrt(Point.X * Point.X + Point.Z * Point.Z);
        if (fabs(Point.Y) > ship->Radius() && Adjacent > ship->Radius())
            Result.pitch = atan(Point.Y / Adjacent);

#if PLATFORM_WINDOWS
        if (!_finite(Result.yaw))   Result.yaw = 0;
        if (!_finite(Result.pitch)) Result.pitch = 0;
#else
        if (!isfinite(Result.yaw))   Result.yaw = 0;
        if (!isfinite(Result.pitch)) Result.pitch = 0;
#endif

        return Result;
    }

    return SteerAI::Seek(Point);
}

Steer
NavAI::Flee(const FVector& Point)
{
    if (ship && ship->IsStarship()) {
        Steer Result = Seek(Point);
        Result.yaw += PI;
        Result.pitch *= -1;
        return Result;
    }

    return SteerAI::Flee(Point);
}

Steer
NavAI::Avoid(const FVector& Point, float Radius)
{
    if (ship && ship->IsStarship()) {
        Steer Result = Seek(Point);

        if ((Point | ship->BeamLine()) > 0)
            Result.yaw -= PI / 2;
        else
            Result.yaw += PI / 2;

        (void)Radius;
        return Result;
    }

    return SteerAI::Avoid(Point, Radius);
}

// +--------------------------------------------------------------------+

Steer
NavAI::AvoidTerrain()
{
    Steer Avoid;

    terrain_warning = 0;

    if (!ship || !ship->GetRegion() || !ship->GetRegion()->IsActive() ||
        takeoff || (navpt && navpt->GetAction() == INSTRUCTION_ACTION::LAUNCH))
        return Avoid;

    if (ship->IsAirborne() && ship->GetFlightPhase() == Ship::ACTIVE) {
        // too low?
        if (ship->AltitudeAGL() < 1000) {
            terrain_warning = 1;
            ship->SetDirectorInfo("Too Low");

            // way too low?
            if (ship->AltitudeAGL() < 750) {
                ship->SetDirectorInfo("Way Too Low!");
            }

            // where will we be?
            FVector SelfPt = ship->Location() + ship->Velocity() + FVector(0, 10e3, 0);

            // transform into camera coords:
            FVector Obj = Transform(SelfPt);

            // pull up!
            Avoid = Seek(Obj);
        }
    }

    return Avoid;
}
