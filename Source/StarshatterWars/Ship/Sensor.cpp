/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Sensor.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR: John DiCamillo
    ORIGINAL STUDIO: Destroyer Studios LLC

    OVERVIEW
    ========
    Integrated (Passive and Active) Sensor Package class implementation
*/

#include "Sensor.h"
#include "CoreMinimal.h"          // UE_LOG, FMemory, FVector
#include "Math/Vector.h"          // FVector

#include "SimContact.h"
#include "SimElement.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimShot.h"
#include "Drone.h"
#include "WeaponDesign.h"
#include "Sim.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "GameStructs.h"

#include "Game.h"

// +----------------------------------------------------------------------+

const double SENSOR_THRESHOLD = 0.25;

// +----------------------------------------------------------------------+

Sensor::Sensor()
    : SimSystem(SYSTEM_CATEGORY::SENSOR, 1, "Dual Sensor Pkg", 1, 10, 10, 10),
    mode(ESensorMode::STD),
    target(0),
    nsettings(0),
    range_index(0)
{
    name = "Sensor";
    abrv = "Sensor";

    SetMode(mode);
    power_flags = POWER_WATTS;

    FMemory::Memzero(range_settings, sizeof(range_settings));
}

// +----------------------------------------------------------------------+

Sensor::Sensor(const Sensor& s)
    : SimSystem(s),
    mode(ESensorMode::STD),
    target(0),
    nsettings(s.nsettings),
    range_index(0)
{
    Mount(s);

    SetMode(mode);
    power_flags = POWER_WATTS;

    FMemory::Memcpy(range_settings, s.range_settings, sizeof(range_settings));

    if (nsettings)
        range_index = nsettings - 1;
}

// +--------------------------------------------------------------------+

Sensor::~Sensor()
{
    ClearAllContacts();
}

// +--------------------------------------------------------------------+

void Sensor::ClearAllContacts()
{
    contacts.destroy();
}

// +--------------------------------------------------------------------+

double Sensor::GetBeamLimit() const
{
    if (mode == ESensorMode::ACM)
        return 15 * DEGREES;

    if (mode <= ESensorMode::GM)
        return 45 * DEGREES;

    return 175 * DEGREES;
}

double Sensor::GetBeamRange() const
{
    return (double)range_settings[range_index];
}

void Sensor::IncreaseRange()
{
    if (range_index < nsettings - 1)
        range_index++;
    else
        range_index = nsettings - 1;
}

void Sensor::DecreaseRange()
{
    if (range_index > 0)
        range_index--;
    else
        range_index = 0;
}

void Sensor::AddRange(double r)
{
    if (nsettings < 8)
        range_settings[nsettings++] = (float)r;

    range_index = nsettings - 1;
}

void Sensor::SetMode(ESensorMode m)
{
    if (mode != m) {
        // dump the contact list when changing in/out of GM:
        if (mode == ESensorMode::GM || m == ESensorMode::GM)
            ClearAllContacts();

        // dump the current target on mode changes:
        if (m <= ESensorMode::GM) {
            if (ship)
                ship->DropTarget();

            Ignore(target);
            target = 0;
        }
    }

    mode = m;
}

// +----------------------------------------------------------------------+

bool Sensor::Update(SimObject* obj)
{
    if (obj == target) {
        target = 0;
    }

    return SimObserver::Update(obj);
}

const char* Sensor::GetObserverName() const
{
    return "Sensor";
}

// +--------------------------------------------------------------------+

void Sensor::ExecFrame(double seconds)
{
    if (Game::Paused())
        return;

    SimSystem::ExecFrame(seconds);

    if (!IsPowerOn() || energy <= 0) {
        ClearAllContacts();
        return;
    }

    if (ship && ship->GetAIMode() < 2) {
        // just pretend to work this frame:
        energy = 0.0f;
        return;
    }

    if (ship && ship->GetRegion()) {
        const Camera* cam = &ship->GetCam();
        double az1 = -45 * DEGREES;
        double az2 = 45 * DEGREES;

        if (mode > ESensorMode::GM) {
            az1 = -175 * DEGREES;
            az2 = 175 * DEGREES;
        }

        ListIter<Ship> ship_iter(ship->GetRegion()->GetShips());
        while (++ship_iter) {
            Ship* c_ship = ship_iter.value();

            if (c_ship != ship) {
                ProcessContact(c_ship, az1, az2);
            }
            else {
                SimContact* c = FindContact(c_ship);

                if (!c) {
                    c = new SimContact(c_ship, 0.0f, 0.0f);
                    contacts.append(c);
                }

                // update track:
                if (c) {
                    c->loc = c_ship->GetLocation();
                    c->d_pas = 2.0f;
                    c->d_act = 2.0f;

                    c->UpdateTrack();
                }
            }
        }

        ListIter<SimShot> threat_iter(ship->GetThreatList());
        while (++threat_iter) {
            ProcessContact(threat_iter.value(), az1, az2);
        }

        ListIter<Drone> drone_iter(ship->GetRegion()->GetDrones());
        while (++drone_iter) {
            ProcessContact(drone_iter.value(), az1, az2);
        }

        List<SimContact>& track_list = ship->GetRegion()->GetTrackList(ship->GetIFF());
        ListIter<SimContact> c_iter(contacts);

        while (++c_iter) {
            SimContact* contact = c_iter.value();
            Ship* c_ship = contact->GetShip();
            SimShot* c_shot = contact->GetShot();
            double c_life = -1;

            if (c_ship) {
                c_life = c_ship->GetLife();

                // look for quantum jumps and orbit transitions:
                if (c_ship->GetRegion() != ship->GetRegion())
                    c_life = 0;
            }
            else if (c_shot) {
                c_life = c_shot->GetLife();
            }
            else {
                c_life = 0;
            }

            if (contact->Age() < 0 || c_life == 0) {
                delete c_iter.removeItem();
            }
            else if (ship && ship->GetIFF() >= 0 && ship->GetIFF() < 5) {
                // update shared track database:
                SimContact* t = track_list.find(contact);

                if (c_ship) {
                    if (!t) {
                        SimContact* track = new SimContact(c_ship, contact->d_pas, contact->d_act);
                        track->loc = c_ship->GetLocation();
                        track_list.append(track);
                    }
                    else {
                        t->loc = c_ship->GetLocation();
                        t->Merge(contact);
                        t->UpdateTrack();
                    }
                }
                else if (c_shot) {
                    if (!t) {
                        SimContact* track = new SimContact(c_shot, contact->d_pas, contact->d_act);
                        track->loc = c_shot->GetLocation();
                        track_list.append(track);
                    }
                    else {
                        t->loc = c_shot->GetLocation();
                        t->Merge(contact);
                        t->UpdateTrack();
                    }
                }
            }
        }

        if (mode == ESensorMode::ACM) {
            if (!ship->GetTarget())
                ship->LockTarget(SimObject::SIM_SHIP, true, true);
        }
    }

    energy = 0.0f;
}

// +--------------------------------------------------------------------+

void Sensor::ProcessContact(Ship* c_ship, double az1, double az2)
{
    if (c_ship->IsNetObserver())
        return;

    double sensor_range = GetBeamRange();

    // translate:
    const Camera* cam = &ship->GetCam();
    FVector targ_pt = c_ship->GetLocation() - ship->GetLocation();

    // rotate:
    const double tx = FVector::DotProduct(targ_pt, cam->vrt());
    const double ty = FVector::DotProduct(targ_pt, cam->vup());
    const double tz = FVector::DotProduct(targ_pt, cam->vpn());

    // convert to spherical coords:
    const double rng = targ_pt.Size();
    double az = asin(fabs(tx) / rng);
    double el = asin(fabs(ty) / rng);
    if (tx < 0)
        az = -az;
    if (ty < 0)
        el = -el;

    double min_range = rng;
    Drone* probe = ship->GetProbe();
    bool probescan = false;

    if (ship->GetIFF() == c_ship->GetIFF()) {
        min_range = 1;
    }
    else if (probe) {
        FVector probe_pt = c_ship->GetLocation() - probe->GetLocation();
        double prng = probe_pt.Size();

        if (prng < probe->GetDesign()->GetLethalRadius() && prng < rng) {
            min_range = prng;
            probescan = true;
        }
    }

    bool vis = tz > 1 && (c_ship->GetRadius() / rng > 0.001);
    bool threat =
        (c_ship->GetLife() != 0 &&
            c_ship->GetIFF() &&
            c_ship->GetIFF() != ship->GetIFF() &&
            c_ship->GetEMCON() > 2 &&
            c_ship->IsTracking(ship));

    if (!threat) {
        if (mode == ESensorMode::GM && !c_ship->IsGroundUnit())
            return;

        if (mode != ESensorMode::GM && c_ship->IsGroundUnit())
            return;

        if (min_range > sensor_range || min_range > c_ship->Design()->detet) {
            if (c_ship == target) {
                ship->DropTarget();
                Ignore(target);
                target = 0;
            }

            return;
        }
    }

    // clip:
    if (threat || vis || mode >= ESensorMode::PST || tz > 1) {

        // correct az/el for back hemisphere:
        if (tz < 0) {
            if (az < 0)
                az = -PI - az;
            else
                az = PI - az;
        }

        double d_pas = 0;
        double d_act = 0;
        double effectivity = energy / capacity * availability;

        // did this contact get scanned this frame?
        if (effectivity > SENSOR_THRESHOLD) {
            if (az >= az1 && az <= az2 && (mode >= ESensorMode::PST || fabs(el) < 45 * DEGREES)) {
                double passive_range_limit = 500e3;
                if (c_ship->Design()->detet > passive_range_limit)
                    passive_range_limit = c_ship->Design()->detet;

                d_pas = c_ship->PCS() * effectivity * (1 - min_range / passive_range_limit);

                if (d_pas < 0)
                    d_pas = 0;

                if (probescan) {
                    double max_range = probe->GetDesign()->GetLethalRadius();
                    d_act = c_ship->ACS() * (1 - min_range / max_range);
                }
                else if (mode != ESensorMode::PAS && mode != ESensorMode::PST) {
                    double max_range = sensor_range;
                    d_act = c_ship->ACS() * effectivity * (1 - min_range / max_range);
                }

                if (d_act < 0)
                    d_act = 0;
            }
        }

        // yes, update or add new contact:
        if (threat || vis || d_pas > SENSOR_THRESHOLD || d_act > SENSOR_THRESHOLD) {
            SimElement* elem = c_ship->GetElement();
            CombatUnit* unit = c_ship->GetCombatUnit();

            if (elem && ship && elem->GetIFF() != ship->GetIFF() && elem->IntelLevel() < Intel::LOCATED) {
                elem->SetIntelLevel(Intel::LOCATED);
            }

            if (unit && ship && unit->GetIFF() != ship->GetIFF()) {
                CombatGroup* group = unit->GetCombatGroup();

                if (group && group->GetIntelLevel() < Intel::LOCATED &&
                    group->GetIntelLevel() > Intel::RESERVE) {
                    group->SetIntelLevel(Intel::LOCATED);
                }
            }

            SimContact* c = FindContact(c_ship);

            if (!c) {
                c = new SimContact(c_ship, 0.0f, 0.0f);
                contacts.append(c);
            }

            // update track:
            if (c) {
                c->loc = c_ship->GetLocation();
                c->d_pas = (float)d_pas;
                c->d_act = (float)d_act;
                c->probe = probescan;

                c->UpdateTrack();
            }
        }
    }
}

// +--------------------------------------------------------------------+

void Sensor::ProcessContact(SimShot* c_shot, double az1, double az2)
{
    double sensor_range = GetBeamRange();

    if (c_shot->IsPrimary() || c_shot->IsDecoy())
        return;

    // translate:
    const Camera* cam = &ship->GetCam();
    FVector targ_pt = c_shot->GetLocation() - ship->GetLocation();

    // rotate:
    const double tx = FVector::DotProduct(targ_pt, cam->vrt());
    const double ty = FVector::DotProduct(targ_pt, cam->vup());
    const double tz = FVector::DotProduct(targ_pt, cam->vpn());

    // convert to spherical coords:
    const double rng = targ_pt.Size();
    double az = asin(fabs(tx) / rng);
    double el = asin(fabs(ty) / rng);
    if (tx < 0)
        az = -az;
    if (ty < 0)
        el = -el;

    bool vis = tz > 1 && (c_shot->GetRadius() / rng > 0.001);
    bool threat = (c_shot->IsTracking(ship));

    // clip:
    if (threat || vis || ((mode >= ESensorMode::PST || tz > 1) && rng <= sensor_range)) {

        // correct az/el for back hemisphere:
        if (tz < 0) {
            if (az < 0)
                az = -PI - az;
            else
                az = PI - az;
        }

        double d_pas = 0;
        double d_act = 0;
        double effectivity = energy / capacity * availability;

        // did this contact get scanned this frame?
        if (effectivity > SENSOR_THRESHOLD) {
            if (az >= az1 && az <= az2 && (mode >= ESensorMode::PST || fabs(el) < 45 * DEGREES)) {
                if (rng < sensor_range / 2)
                    d_pas = 1.5;
                else
                    d_pas = 0.5;

                if (mode != ESensorMode::PAS && mode != ESensorMode::PST)
                    d_act = effectivity * (1 - rng / sensor_range);

                if (d_act < 0)
                    d_act = 0;
            }
        }

        // yes, update or add new contact:
        if (threat || vis || d_pas > SENSOR_THRESHOLD || d_act > SENSOR_THRESHOLD) {
            SimContact* c = FindContact(c_shot);

            if (!c) {
                c = new SimContact(c_shot, 0.0f, 0.0f);
                contacts.append(c);
            }

            // update track:
            if (c) {
                c->loc = c_shot->GetLocation();
                c->d_pas = (float)d_pas;
                c->d_act = (float)d_act;

                c->UpdateTrack();
            }
        }
    }
}

SimContact* Sensor::FindContact(Ship* s)
{
    ListIter<SimContact> iter(contacts);
    while (++iter) {
        SimContact* c = iter.value();
        if (c->GetShip() == s)
            return c;
    }

    return 0;
}

SimContact* Sensor::FindContact(SimShot* s)
{
    ListIter<SimContact> iter(contacts);
    while (++iter) {
        SimContact* c = iter.value();
        if (c->GetShot() == s)
            return c;
    }

    return 0;
}

// +--------------------------------------------------------------------+

bool Sensor::IsTracking(SimObject* tgt)
{
    if (tgt && mode != ESensorMode::GM && mode != ESensorMode::PAS && mode != ESensorMode::PST && IsPowerOn()) {
        if (tgt == target)
            return true;

        SimContact* c = 0;

        if (tgt->GetType() == SimObject::SIM_SHIP) {
            c = FindContact((Ship*)tgt);
        }
        else {
            c = FindContact((SimShot*)tgt);
        }

        return (c != 0 && c->ActReturn() > SENSOR_THRESHOLD && !c->IsProbed());
    }

    return false;
}

// +--------------------------------------------------------------------+

const double sensor_lock_threshold = 0.5;

struct TargetOffset {
    static const char* TYPENAME() { return "TargetOffset"; }

    SimObject* target;
    double offset;

    TargetOffset() : target(0), offset(10) {}
    TargetOffset(SimObject* t, double o) : target(t), offset(o) {}
    int operator< (const TargetOffset& o) const { return offset < o.offset; }
    int operator<=(const TargetOffset& o) const { return offset <= o.offset; }
    int operator==(const TargetOffset& o) const { return offset == o.offset; }
};

SimObject* Sensor::LockTarget(int obj_type, bool closest, bool hostile)
{
    if (!ship || ship->GetEMCON() < 3) {
        Ignore(target);
        target = 0;
        return target;
    }

    SimObject* test = 0;
    ListIter<SimContact> contact(ship->ContactList());

    List<TargetOffset> targets;

    while (++contact) {
        if (obj_type == SimObject::SIM_SHIP)
            test = contact->GetShip();
        else
            test = contact->GetShot();

        if (!test)
            continue;

        // do not target own missiles:
        if (contact->GetShot() && contact->GetShot()->Owner() == ship)
            continue;

        double tgt_range = contact->Range(ship);

        // do not target ships outside of detet range:
        if (contact->GetShip() && contact->GetShip()->Design()->detet < tgt_range)
            continue;

        double d_pas = contact->PasReturn();
        double d_act = contact->ActReturn();
        bool vis = contact->Visible(ship) || contact->Threat(ship);

        if (!vis && d_pas < sensor_lock_threshold && d_act < sensor_lock_threshold)
            continue;

        if (closest) {
            if (hostile && (contact->GetIFF(ship) == 0 || contact->GetIFF(ship) == ship->GetIFF()))
                continue;

            targets.append(new TargetOffset(test, tgt_range));
        }

        // clip:
        else if (contact->InFront(ship)) {
            double az, el, rng;
            contact->GetBearing(ship, az, el, rng);
            az = fabs(az / PI);
            el = fabs(el / PI);

            if (az <= 0.2 && el <= 0.2)
                targets.append(new TargetOffset(test, az + el));
        }
    }

    targets.sort();

    if (target) {
        int index = 100000;
        int i = 0;

        if (targets.size() > 0) {
            ListIter<TargetOffset> iter(targets);
            while (++iter) {
                if (iter->target == target) {
                    index = i;
                    break;
                }
                i++;
            }

            if (index < targets.size() - 1)
                index++;
            else
                index = 0;

            target = targets[index]->target;
            Observe(target);
        }
    }
    else if (targets.size() > 0) {
        target = targets[0]->target;
        Observe(target);
    }
    else {
        target = 0;
    }

    targets.destroy();

    if (target && mode < ESensorMode::STD)
        mode = ESensorMode::STD;

    return target;
}

// +--------------------------------------------------------------------+

SimObject* Sensor::LockTarget(SimObject* candidate)
{
    Ignore(target);
    target = 0;

    if (ship->GetEMCON() < 3)
        return target;

    if (!candidate)
        return target;

    int candidate_type = candidate->GetType();
    SimObject* test = 0;
    ListIter<SimContact> contact(ship->ContactList());

    while (++contact) {
        if (candidate_type == SimObject::SIM_SHIP)
            test = contact->GetShip();
        else
            test = contact->GetShot();

        if (test == candidate) {
            double d_pas = contact->PasReturn();
            double d_act = contact->ActReturn();
            bool vis = contact->Visible(ship) || contact->Threat(ship);

            if (vis || d_pas > sensor_lock_threshold || d_act > sensor_lock_threshold) {
                target = test;
                Observe(target);
            }

            break;
        }
    }

    if (target && mode < ESensorMode::STD)
        mode = ESensorMode::STD;

    return target;
}

// +--------------------------------------------------------------------+

SimObject* Sensor::AcquirePassiveTargetForMissile()
{
    SimObject* pick = 0;
    double min_off = 2;

    ListIter<SimContact> contact(ship->ContactList());

    while (++contact) {
        SimObject* test = contact->GetShip();
        double d = contact->PasReturn();

        if (d < 1)
            continue;

        // clip:
        if (contact->InFront(ship)) {
            double az, el, rng;

            contact->GetBearing(ship, az, el, rng);
            az = fabs(az / PI);
            el = fabs(el / PI);

            if (az + el < min_off) {
                min_off = az + el;
                pick = test;
            }
        }
    }

    return pick;
}

// +--------------------------------------------------------------------+

SimObject* Sensor::AcquireActiveTargetForMissile()
{
    SimObject* pick = 0;
    double min_off = 2;

    ListIter<SimContact> contact(ship->ContactList());

    while (++contact) {
        SimObject* test = contact->GetShip();
        double d = contact->ActReturn();

        if (d < 1)
            continue;

        if (contact->InFront(ship)) {
            double az, el, rng;

            contact->GetBearing(ship, az, el, rng);
            az = fabs(az / PI);
            el = fabs(el / PI);

            if (az + el < min_off) {
                min_off = az + el;
                pick = test;
            }
        }
    }

    return pick;
}

// +--------------------------------------------------------------------+

void Sensor::DoEMCON(int index)
{
    int e = GetEMCONPower(index);

    if (power_level * 100 > e || emcon != index) {
        if (e == 0) {
            SetPowerOff();
        }
        else if (emcon != index) {
            SetPowerOn();

            if (power_level * 100 > e) {
                SetPowerLevel(e);
            }

            if (emcon == 3) {
                if (GetMode() < ESensorMode::PST)
                    SetMode(ESensorMode::STD);
                else
                    SetMode(ESensorMode::CST);
            }
            else {
                ESensorMode m = GetMode();
                if (m < ESensorMode::PST && m > ESensorMode::PAS)
                    SetMode(ESensorMode::PAS);
                else if (m == ESensorMode::CST)
                    SetMode(ESensorMode::PST);
            }
        }
    }

    emcon = index;
}

void Sensor::SetRangeSettings(const TArray<float>& InRanges)
{
    nsettings = 0;

    for (int32 i = 0; i < 8; ++i)
    {
        range_settings[i] = 0.0f;
    }

    const int32 Count = FMath::Min(InRanges.Num(), 8);

    for (int32 i = 0; i < Count; ++i)
    {
        range_settings[i] = InRanges[i];
    }

    nsettings = Count;
    range_index = Count > 0 ? 0 : -1;
}

ESensorMode Sensor::GetNextSensorMode(ESensorMode Mode)
{
    switch (Mode)
    {
    case ESensorMode::PAS: return ESensorMode::STD;
    case ESensorMode::STD: return ESensorMode::ACM;
    case ESensorMode::ACM: return ESensorMode::GM;
    case ESensorMode::GM:  return ESensorMode::PST;
    case ESensorMode::PST: return ESensorMode::CST;
    default:               return ESensorMode::PAS;
    }
}

