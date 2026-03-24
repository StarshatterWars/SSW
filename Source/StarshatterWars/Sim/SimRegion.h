/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025–2026. All Rights Reserved.

    ORIGINAL WORK:
    Starshatter 4.5
    Copyright © 1997–2004 Destroyer Studios LLC
    Original Author: John DiCamillo

    SUBSYSTEM:    StarshatterWars
    FILE:         SimRegion.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Simulation Region class (UE-compliant, FVector-only)
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "Text.h"

// Unreal
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Sim;
class SimObject;
class SimShot;
class SimContact;

class StarSystem;
class OrbitalRegion;
class TerrainRegion;

class Asteroid;
class Ship;
class Drone;
class Explosion;
class Debris;

class Grid;
class Terrain;

// +--------------------------------------------------------------------+

class SimRegion
{
    friend class Sim;

public:
    static const char* TYPENAME() { return "SimRegion"; }

    enum { REAL_SPACE, AIR_SPACE };

    SimRegion(Sim* sim, const char* name, int type);
    SimRegion(Sim* sim, OrbitalRegion* rgn);
    virtual ~SimRegion();

    int operator==(const SimRegion& r) const { return (sim == r.sim) && (name == r.name); }
    int operator<(const SimRegion& r) const;
    int operator<=(const SimRegion& r) const;

    // Lifecycle:
    virtual void Activate();
    virtual void Deactivate();
    virtual void ExecFrame(double seconds);

    void  ShowGrid(int show = true);
    void  NextView();

    void CommitMission();

    bool  IsActive()   const { return active; }
    bool  IsAirSpace() const { return type == AIR_SPACE; }
    bool  IsOrbital()  const { return type == REAL_SPACE; }

    bool  CanTimeSkip() const;
    void  ResolveTimeSkip(double seconds);

    // Objects:
    virtual void InsertObject(Ship* ship);
    virtual void InsertObject(SimShot* shot);
    virtual void InsertObject(Explosion* explosion);
    virtual void InsertObject(Debris* debris);
    virtual void InsertObject(Asteroid* asteroid);

    Ship* FindShip(const char* name);
    Ship* FindShipByObjID(uint32 objid);
    SimShot* FindShotByObjID(uint32 objid);

    // Player:
    Ship* GetPlayerShip() { return player_ship; }
    void  SetPlayerShip(Ship* ship);

    // Region data:
    const char* GetName() const { return name; }
    int             GetType() const { return type; }
    StarSystem* GetSystem() { return star_system; }
    OrbitalRegion* GetOrbitalRegion() { return orbital_region; }
    Terrain* GetTerrain() { return terrain; }

    FVector         GetLocation() const { return location; }

    int             GetNumShips() { return ships.size(); }
    List<Ship>& GetShips() { return ships; }
    List<Ship>& GetCarriers() { return carriers; }
    List<SimShot>& GetShots() { return shots; }
    List<Drone>& GetDrones() { return drones; }
    List<Debris>& GetRocks() { return debris; }
    List<Asteroid>& GetRoids() { return asteroids; }
    List<Explosion>& GetExplosions() { return explosions; }
    List<SimRegion>& GetLinks() { return links; }

    // Selection:
    void           SetSelection(Ship* s);
    bool           IsSelected(Ship* s);
    ListIter<Ship> GetSelection();
    void           ClearSelection();
    void           AddSelection(Ship* s);

    // Tracking:
    List<SimContact>& TrackList(int iff);

protected:
    // Internal simulation pipeline:
    void TranslateObject(SimObject* object);

    void DestroyShips();
    void DestroyShip(Ship* ship);

    void UpdateShips(double seconds);
    void UpdateShots(double seconds);
    void UpdateExplosions(double seconds);
    void UpdateTracks(double seconds);

protected:
    Sim* sim = nullptr;
    Text            name;
    int             type = REAL_SPACE;

    StarSystem* star_system = nullptr;
    OrbitalRegion* orbital_region = nullptr;

    FVector         location = FVector::ZeroVector;

    Grid* grid = nullptr;
    Terrain* terrain = nullptr;

    bool            active = false;
    int             current_view = 0;

    Ship* player_ship = nullptr;

    List<Ship>      ships;
    List<Ship>      carriers;
    List<Ship>      selection;
    List<Ship>      dead_ships;

    List<SimShot>   shots;
    List<Drone>     drones;
    List<Explosion> explosions;
    List<Debris>    debris;
    List<Asteroid>  asteroids;

    List<SimContact> track_database[5];
    List<SimRegion>  links;

    DWORD           sim_time = 0;
    int             ai_index = 0;
};
