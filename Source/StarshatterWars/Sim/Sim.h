/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025–2026. All Rights Reserved.

    ORIGINAL WORK:
    Starshatter 4.5
    Copyright © 1997–2004 Destroyer Studios LLC
    Original Author: John DiCamillo

    SUBSYSTEM:    StarshatterWars
    FILE:         Sim.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Simulation Universe (UE-compliant, non-UObject)
    NOTE: SimRegion is defined in SimRegion.h / SimRegion.cpp
*/

#pragma once

#include "Types.h"
#include "SimUniverse.h"
#include "SimScene.h"
#include "Physical.h"
#include "List.h"
#include "Text.h"

#include "SimRegion.h"     // MOVED OUT: SimRegion now lives here

// Minimal Unreal includes required for FVector / FColor:
#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class SimObject;
class SimObserver;
class SimHyper;
class SimSplash;
class SimElement;
class SimSystem;

class SimContact;
class SimShot;

class StarSystem;
class Orbital;
class OrbitalRegion;
class Asteroid;

class CameraManager;

class Ship;
class ShipDesign;

class Drone;
class Explosion;
class Debris;
class WeaponDesign;
class MotionController;
class Dust;
class Grid;
class Mission;
class MissionElement;
class MissionEvent;
class Hangar;
class FlightDeck;

class Terrain;
class TerrainPatch;

class SimModel;

// +--------------------------------------------------------------------+

class Sim : public SimUniverse
{
public:
    // NOTE: Keep only if Sim uses these directly.
    // Otherwise, prefer SimRegion::REAL_SPACE / SimRegion::AIR_SPACE.
    enum { REAL_SPACE, AIR_SPACE };

    Sim(MotionController* ctrl);
    virtual ~Sim();

    static Sim* GetSim() { return sim; }

    virtual void ExecFrame(double seconds);

    void LoadMission(Mission* msn, bool preload_textures = false);
    void ExecMission();
    void CommitMission();
    void UnloadMission();

    void NextView();
    void ShowGrid(int show = true);
    bool GridShown() const;

    List<StarSystem>& GetSystemList();

    const char* FindAvailCallsign(int IFF);
    SimElement* CreateElement(const char* callsign, int IFF, int type = 0);
    void DestroyElement(SimElement* elem);

    Ship* CreateShip(
        const char* name,
        const char* reg_num,
        ShipDesign* design,
        const char* rgn_name,
        const FVector& loc,
        int IFF = 0,
        int cmd_ai = 0,
        const int* loadout = nullptr
    );

    Ship* FindShip(const char* name, const char* rgn_name = nullptr);

    SimShot* CreateShot(
        const FVector& pos,
        const Camera& shot_cam,
        WeaponDesign* d,
        const Ship* ship = nullptr,
        SimRegion* rgn = nullptr
    );

    Explosion* CreateExplosion(
        const FVector& pos,
        const FVector& vel,
        int type,
        float exp_scale,
        float part_scale,
        SimRegion* rgn = nullptr,
        SimObject* source = nullptr,
        SimSystem* sys = nullptr
    );

    Debris* CreateDebris(const FVector& pos, const FVector& vel, SimModel* model, double mass, SimRegion* rgn = nullptr);
    Asteroid* CreateAsteroid(const FVector& pos, int type, double mass, SimRegion* rgn = nullptr);

    void CreateSplashDamage(Ship* ship);
    void CreateSplashDamage(SimShot* shot);

    void DestroyShip(Ship* ship);

    // NETWORKING REMOVED: you will add UE networking later.
    // void NetDockShip(Ship* ship, Ship* carrier, FlightDeck* deck);

    virtual Ship* FindShipByObjID(uint32 objid);
    virtual SimShot* FindShotByObjID(uint32 objid);

    Mission* GetMission() { return mission; }
    List<MissionEvent>& GetEvents() { return events; }
    List<SimRegion>& GetRegions() { return regions; }

    SimRegion* FindRegion(const char* name);
    SimRegion* FindRegion(OrbitalRegion* rgn);
    SimRegion* FindRegion(const FString& Name); 

    SimRegion* FindNearestSpaceRegion(SimObject* object);
    SimRegion* FindNearestTerrainRegion(SimObject* object);
    SimRegion* FindNearestRegion(SimObject* object, int type);
    SimRegion* FindNearestSpaceRegion(const Orbital* orb);
    SimRegion* FindNearestSpaceRegionAt(const FVector& loc);

    bool ActivateRegion(SimRegion* rgn);

    void RequestHyperJump(
        Ship* obj,
        SimRegion* rgn,
        const FVector& loc,
        int type = 0,
        Ship* fc_src = nullptr,
        Ship* fc_dst = nullptr
    );

    SimRegion* GetActiveRegion() { return active_region; }
    StarSystem* GetStarSystem() { return star_system; }
    SimScene* GetScene() { return scene; }

    Ship* GetPlayerShip();
    SimElement* GetPlayerElement();
    Orbital* FindOrbitalBody(const char* name);

    void SetSelection(Ship* s);
    bool IsSelected(Ship* s);
    ListIter<Ship> GetSelection();
    void ClearSelection();
    void AddSelection(Ship* s);

    void SetTestMode(bool t = true);

    bool IsTestMode() const { return test_mode; }
    bool IsActive()   const;
    bool IsComplete() const;

    MotionController* GetControls() const { return ctrl; }

    SimElement* FindElement(const char* name);
    int GetAssignedElements(SimElement* elem, List<SimElement>& assigned);
    List<SimElement>& GetElements() { return elements; }
    ListIter<MissionElement> GetMissionElements();

    MissionElement* CreateMissionElement(SimElement* elem);

    Hangar* FindSquadron(const char* name, int& index);

    void SkipCutscene();
    void ResolveTimeSkip(double seconds);

    void ResolveHyperList();
    void ResolveSplashList();

    void ExecEvents(double seconds);
    void ProcessEventTrigger(int type, int event_id = 0, const char* ship = nullptr, int param = 0);

    double MissionClock() const;
    uint32 StartTime() const { return start_time; }

protected:
    void CreateRegions();
    void CreateElements();
    void CopyEvents();
    void BuildLinks();

    static Sim* sim;

    SimRegion* active_region = nullptr;
    StarSystem* star_system = nullptr;
    SimScene* scene = nullptr;
    Dust* dust = nullptr;
    CameraManager* cam_dir = nullptr;

    List<SimRegion>      regions;
    List<SimRegion>      rgn_queue;
    List<SimHyper>       jumplist;
    List<SimSplash>      splashlist;
    List<SimElement>     elements;
    List<SimElement>     finished;
    List<MissionEvent>   events;
    List<MissionElement> mission_elements;

    MotionController* ctrl = nullptr;

    bool        test_mode = false;
    bool        grid_shown = false;
    Mission* mission = nullptr;
    uint32      start_time = 0;
};
