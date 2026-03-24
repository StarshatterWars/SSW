/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025–2026. All Rights Reserved.

    ORIGINAL WORK:
    Starshatter 4.5
    Copyright © 1997–2004 Destroyer Studios LLC
    Original Author: John DiCamillo

    SUBSYSTEM:    StarshatterWars
    FILE:         SimRegion.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Simulation Region implementation (FVector-only, no networking, no stubs)
*/

#include "SimRegion.h"
#include "Sim.h"

// Core world types:
#include "StarSystem.h"
//#include "OrbitalRegion.h"
#include "TerrainRegion.h"
#include "Terrain.h"
#include "Grid.h"

// Sim objects:
#include "SimObject.h"
#include "SimElement.h"
#include "Ship.h"
#include "Sensor.h"
#include "SimShot.h"
#include "Drone.h"
#include "Explosion.h"
#include "Debris.h"
#include "Asteroid.h"
#include "SimContact.h"
#include "GameStructs.h"

// Systems:
#include "Game.h"

// Unreal:
#include "Math/UnrealMathUtility.h"

// +--------------------------------------------------------------------+
// Local helpers (single edit point if your API differs)
// +--------------------------------------------------------------------+

static FORCEINLINE int ClampIFF(int iff)
{
    if (iff < 0) return 0;
    if (iff > 4) return 4;
    return iff;
}

static FORCEINLINE bool IsDeadShip(const Ship* s)
{
    if (!s) return true;
    if (s->Life() <= 0) return true;
    if (s->IsDead())    return true;
    if (s->IsDying())   return true;
    return false;
}

static FORCEINLINE bool IsDeadShot(const SimShot* s)
{
    if (!s) return true;
    if (s->Life() <= 0) return true;
    return false;
}

static FORCEINLINE bool IsDeadExplosion(const Explosion* e)
{
    if (!e) return true;
    if (e->Life() <= 0) return true;
    return false;
}

// IMPORTANT: ObjID getter name can vary across your port.
// If your build fails here, change ONLY these helpers.
static FORCEINLINE uint32 GetObjID_Ship(const Ship* s)
{
    return s ? (uint32)s->GetObjID() : 0;
}

static FORCEINLINE uint32 GetObjID_Shot(const SimShot* s)
{
    return s ? (uint32)s->GetObjID() : 0;
}

// +--------------------------------------------------------------------+

int SimRegion::operator<(const SimRegion& r) const
{
    return name < r.name;
}

int SimRegion::operator<=(const SimRegion& r) const
{
    return (*this < r) || (*this == r);
}

// +--------------------------------------------------------------------+

SimRegion::SimRegion(Sim* s, const char* n, int t)
    : sim(s), name(n), type(t)
{
    if (sim)
        star_system = sim->GetStarSystem();

    orbital_region = nullptr;
    location = FVector::ZeroVector;
    grid = nullptr;
    terrain = nullptr;
    active = false;
    player_ship = nullptr;
    current_view = 0;
    sim_time = 0;
    ai_index = 0;
}

SimRegion::SimRegion(Sim* SimPtr, OrbitalRegion* OrbitalRegionPtr)
    : sim(SimPtr),
    orbital_region(OrbitalRegionPtr),
    type(REAL_SPACE)
{
    star_system = nullptr;
    location = FVector::ZeroVector;
    grid = nullptr;
    terrain = nullptr;
    active = false;
    player_ship = nullptr;
    current_view = 0;
    sim_time = 0;
    ai_index = 0;

    if (orbital_region) {
        star_system = orbital_region->System();
        name = orbital_region->Name();
        location = orbital_region->Location();

        grid = new Grid(
            (int32)orbital_region->Radius(),
            (int32)orbital_region->GridSpace()
        );

        if (orbital_region->Type() == Orbital::TERRAIN) {
            TerrainRegion* TerrainRegionPtr = (TerrainRegion*)orbital_region;
            terrain = new Terrain(TerrainRegionPtr);
            type = AIR_SPACE;
        }
        else {
            type = REAL_SPACE;
        }
    }
    else {
        name = Game::GetText("Unknown");
        location = FVector::ZeroVector;
    }
}

SimRegion::~SimRegion()
{
    player_ship = nullptr;

    delete terrain;
    terrain = nullptr;

    delete grid;
    grid = nullptr;

    ships.clear();
    carriers.clear();
    selection.clear();
    dead_ships.clear();
    shots.clear();
    drones.clear();
    explosions.clear();
    debris.clear();
    asteroids.clear();

    for (int i = 0; i < 5; ++i)
        track_database[i].clear();

    links.clear();
}

// +--------------------------------------------------------------------+

void SimRegion::Activate()
{
    if (active)
        return;

    active = true;

    if (star_system && orbital_region) {
        star_system->SetActiveRegion(orbital_region);
    }
}

void SimRegion::Deactivate()
{
    if (!active)
        return;

    active = false;

    for (int i = 0; i < 5; ++i)
        track_database[i].clear();
}

// +--------------------------------------------------------------------+

void SimRegion::ExecFrame(double seconds)
{
    if (seconds <= 0)
        return;

    UpdateShips(seconds);
    UpdateShots(seconds);
    UpdateExplosions(seconds);
    UpdateTracks(seconds);

    DestroyShips();

    sim_time += (DWORD)(seconds * 1000.0);
}

// +--------------------------------------------------------------------+

void SimRegion::ShowGrid(int show)
{
    if (grid)
        grid->ShowGrid(show ? true : false);
}

void SimRegion::NextView()
{
    current_view++;
    if (current_view > 3)
        current_view = 0;
}

// +--------------------------------------------------------------------+

bool SimRegion::CanTimeSkip() const
{
    if (shots.size() > 0)  return false;
    if (drones.size() > 0) return false;

    for (int i = 0; i < ships.size(); ++i) {
        Ship* s = ships[i];
        if (!s) continue;

        if (s->IsInCombat())
            return false;

        if (s->IsDying() || s->IsDead())
            return false;
    }

    return true;
}

void SimRegion::ResolveTimeSkip(double seconds)
{
    if (seconds <= 0)
        return;

    const double Step = 1.0;
    double Remaining = seconds;

    while (Remaining > 0.0) {
        const double Dt = (Remaining > Step) ? Step : Remaining;

        UpdateShips(Dt);
        UpdateShots(Dt);
        UpdateExplosions(Dt);
        UpdateTracks(Dt);
        DestroyShips();

        sim_time += (DWORD)(Dt * 1000.0);
        Remaining -= Dt;
    }
}

// +--------------------------------------------------------------------+

Ship* SimRegion::FindShip(const char* n)
{
    if (!n || !*n)
        return nullptr;

    for (int i = 0; i < ships.size(); i++) {
        Ship* s = ships[i];
        if (!s) continue;

        if (!_stricmp(s->Name(), n))
            return s;
    }

    return nullptr;
}

Ship* SimRegion::FindShipByObjID(uint32 objid)
{
    if (!objid)
        return nullptr;

    for (int i = 0; i < ships.size(); i++) {
        Ship* s = ships[i];
        if (!s) continue;

        if (GetObjID_Ship(s) == objid)
            return s;
    }

    return nullptr;
}

SimShot* SimRegion::FindShotByObjID(uint32 objid)
{
    if (!objid)
        return nullptr;

    for (int i = 0; i < drones.size(); i++) {
        Drone* d = drones[i];
        if (!d) continue;

        if (GetObjID_Shot((SimShot*)d) == objid)
            return (SimShot*)d;
    }

    for (int i = 0; i < shots.size(); i++) {
        SimShot* s = shots[i];
        if (!s) continue;

        if (GetObjID_Shot(s) == objid)
            return s;
    }

    return nullptr;
}

// +--------------------------------------------------------------------+

void SimRegion::InsertObject(Ship* s)
{
    if (!s)
        return;

    if (!ships.contains(s))
        ships.append(s);

    if (s->NumFlightDecks() > 0) {
        if (!carriers.contains(s))
            carriers.append(s);
    }

    TranslateObject((SimObject*)s);

    if (!player_ship) {
        SimElement* elem = s->GetElement();
        if (elem && elem->GetPlayer() > 0) {
            player_ship = s;
        }
    }
}

void SimRegion::InsertObject(SimShot* shot)
{
    if (!shot)
        return;

    if (Drone* d = dynamic_cast<Drone*>(shot)) {
        if (!drones.contains(d))
            drones.append(d);
    }
    else {
        if (!shots.contains(shot))
            shots.append(shot);
    }

    TranslateObject((SimObject*)shot);
}

void SimRegion::InsertObject(Explosion* e)
{
    if (!e)
        return;

    if (!explosions.contains(e))
        explosions.append(e);

    TranslateObject((SimObject*)e);
}

void SimRegion::InsertObject(Debris* d)
{
    if (!d)
        return;

    if (!debris.contains(d))
        debris.append(d);

    TranslateObject((SimObject*)d);
}

void SimRegion::InsertObject(Asteroid* a)
{
    if (!a)
        return;

    if (!asteroids.contains(a))
        asteroids.append(a);

    TranslateObject((SimObject*)a);
}

// +--------------------------------------------------------------------+

void SimRegion::SetPlayerShip(Ship* s)
{
    player_ship = s;
}

// +--------------------------------------------------------------------+
// Selection
// +--------------------------------------------------------------------+

void SimRegion::SetSelection(Ship* s)
{
    selection.clear();
    if (s)
        selection.append(s);
}

bool SimRegion::IsSelected(Ship* s)
{
    return s && selection.contains(s);
}

ListIter<Ship> SimRegion::GetSelection()
{
    return selection;
}

void SimRegion::ClearSelection()
{
    selection.clear();
}

void SimRegion::AddSelection(Ship* s)
{
    if (s && !selection.contains(s))
        selection.append(s);
}

// +--------------------------------------------------------------------+
// Tracking
// +--------------------------------------------------------------------+

List<SimContact>& SimRegion::TrackList(int iff)
{
    return track_database[ClampIFF(iff)];
}

// +--------------------------------------------------------------------+
// Internal mechanics
// +--------------------------------------------------------------------+

void SimRegion::TranslateObject(SimObject* obj)
{
    if (!obj)
        return;

    obj->SetRegion(this);

    // Migration-safe: do not mutate coordinates here.
    // Treat SimObject::Location() as region-local and use region->GetLocation() when needed.
}

// +--------------------------------------------------------------------+

void SimRegion::UpdateShips(double seconds)
{
    if (ships.size() == 0)
        return;

    carriers.clear();

    for (int i = 0; i < ships.size(); /* manual */) {
        Ship* s = ships[i];

        if (!s || IsDeadShip(s)) {
            if (s && !dead_ships.contains(s))
                dead_ships.append(s);

            ships.removeIndex(i);
            continue;
        }

        if (s->GetRegion() != this)
            s->SetRegion(this);

        s->ExecFrame(seconds);

        if (s->NumFlightDecks() > 0)
            carriers.append(s);

        if (IsDeadShip(s)) {
            if (!dead_ships.contains(s))
                dead_ships.append(s);

            ships.removeIndex(i);
            continue;
        }

        ++i;
    }

    if (player_ship && player_ship->GetRegion() != this)
        player_ship = nullptr;

    if (player_ship && IsDeadShip(player_ship))
        player_ship = nullptr;
}

void SimRegion::UpdateShots(double seconds)
{
    for (int i = 0; i < shots.size(); /* manual */) {
        SimShot* s = shots[i];

        if (!s || IsDeadShot(s)) {
            shots.removeIndex(i);
            continue;
        }

        if (s->GetRegion() != this)
            s->SetRegion(this);

        s->ExecFrame(seconds);

        if (IsDeadShot(s)) {
            shots.removeIndex(i);
            continue;
        }

        ++i;
    }

    for (int i = 0; i < drones.size(); /* manual */) {
        Drone* d = drones[i];

        if (!d || IsDeadShot((SimShot*)d)) {
            drones.removeIndex(i);
            continue;
        }

        if (d->GetRegion() != this)
            d->SetRegion(this);

        d->ExecFrame(seconds);

        if (IsDeadShot((SimShot*)d)) {
            drones.removeIndex(i);
            continue;
        }

        ++i;
    }
}

void SimRegion::UpdateExplosions(double seconds)
{
    for (int i = 0; i < explosions.size(); /* manual */) {
        Explosion* e = explosions[i];

        if (!e || IsDeadExplosion(e)) {
            explosions.removeIndex(i);
            continue;
        }

        if (e->GetRegion() != this)
            e->SetRegion(this);

        e->ExecFrame(seconds);

        if (IsDeadExplosion(e)) {
            explosions.removeIndex(i);
            continue;
        }

        ++i;
    }
}

void SimRegion::UpdateTracks(double seconds)
{
    (void)seconds;

    for (int k = 0; k < 5; ++k)
        track_database[k].clear();

    for (int i = 0; i < ships.size(); ++i) {
        Ship* s = ships[i];
        if (!s || IsDeadShip(s))
            continue;

        const int iff = ClampIFF(s->GetIFF());

        // If your SimContact API differs, fix here once.
        SimContact* c = new SimContact(s, 1.0f, 0.0f);
        track_database[iff].append(c);
    }
}

// +--------------------------------------------------------------------+

void SimRegion::DestroyShips()
{
    if (dead_ships.size() == 0)
        return;

    for (int i = 0; i < dead_ships.size(); ++i) {
        Ship* s = dead_ships[i];
        if (!s) continue;
        DestroyShip(s);
    }

    dead_ships.clear();
}

void SimRegion::DestroyShip(Ship* ship)
{
    if (!ship)
        return;

    if (ships.contains(ship))
        ships.remove(ship);

    if (carriers.contains(ship))
        carriers.remove(ship);

    if (selection.contains(ship))
        selection.remove(ship);

    if (player_ship == ship)
        player_ship = nullptr;

    delete ship;
}

void SimRegion::CommitMission()
{
    // Region is being finalized at mission end.
    // Purpose: detach volatile runtime state that should not persist past mission end.

    // 1) Clear selection + player references (avoid dangling pointers later):
    ClearSelection();

    if (player_ship)
    {
        // Ensure the ship doesn’t keep region-local transient state:
        Sensor* S = player_ship->GetSensor();
        if (S)
            S->ClearAllContacts();
    }

    player_ship = nullptr;

    // 2) Flush per-ship transient states that should not persist:
    //    (Do NOT delete ships here; Sim/Scene GC handles dead objects separately.)
    ListIter<Ship> s_iter = ships;
    while (++s_iter)
    {
        Ship* S = s_iter.value();
        if (!S) continue;

        // Stop “tracking” / region-local cached lists:
        S->ClearTrack();

        // Clear sensor contacts so they don’t persist into post-mission menus:
        Sensor* SensorPtr = S->GetSensor();
        if (SensorPtr)
            SensorPtr->ClearAllContacts();

        // If you have any per-mission flags that need clearing, do it here.
        // Examples (only if they exist in your Ship class):
        // S->SetAutoNav(false);
        // S->SetInCombat(false);
    }

    // 3) Clean up transient shots/drones lists (if region owns these containers):
    //    If your region uses Scene GC via SetLife(0) etc., mark them.
    ListIter<SimShot> sh_iter = shots;
    while (++sh_iter)
    {
        SimShot* Shot = sh_iter.value();
        if (Shot)
            Shot->SetLife(0);
    }

    ListIter<Drone> d_iter = drones;
    while (++d_iter)
    {
        Drone* D = d_iter.value();
        if (D)
            D->SetLife(0);
    }

    // 4) Terrain/grid end-of-mission housekeeping:
    if (terrain)
    {
        // If your Terrain class has an end-mission/cleanup call, invoke it.
        // Otherwise, at minimum ensure it is not “active”.
        if (terrain && sim && sim->GetScene())
        {
            terrain->Deactivate(*sim->GetScene());
        }
    }

    if (grid)
    {
        // If grid has any dynamic render buffers, reset them.
        // This is harmless even if ShowGrid was used.
        grid->ShowGrid(false);
    }

    // 5) Region is no longer considered active after commit:
    active = false;
}
