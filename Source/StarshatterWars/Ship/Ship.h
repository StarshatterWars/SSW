/*  Project Starshatter 4.5
    Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Ship.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Starship (or space/ground station) class

    UE PORT NOTES
    =============
    - Plain C++ class (notxa UObject)
    - Keeps Starshatter core types (Text, List, Color, etc.)
    - Replaces Bitmap with UTexture2D* (forward-declared)
    - Uses forward declarations aggressively to keep header light
    - Converts Vec3 and Point to FVector (requires minimal UE include)
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "List.h"
#include "DetailSet.h"

#include "Math/Vector.h" // FVector
#include "Math/Color.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class SimShot;
class Drone;

class UTexture2D;

class CombatUnit;
class Computer;
class SimContact;
class Drive;
class SimElement;
class SimSystem;
class SimObject;
class SimDirector;
class Farcaster;
class FlightComputer;
class FlightDeck;
class Hangar;
class InboundSlot;
class Instruction;
class LandingGear;
class MotionController;
class NavLight;
class NavSystem;
class PowerSource;
class QuantumDrive;
class RadioMessage;
class Shield;
class ShieldRep;
class ShipDesign;
class ShipKiller;
class Solid;
class Skin;
class USound;
class Sensor;

class Thruster;
class Weapon;
class WeaponDesign;
class WeaponGroup;

class SimScene;
class SimRegion;
class Sim;

// +--------------------------------------------------------------------+

class Ship : public SimObject, public SimObserver
{
public:
    static const char* TYPENAME() { return "Ship"; }

    enum OP_MODE { DOCKED, ALERT, LOCKED, LAUNCH, TAKEOFF, ACTIVE, APPROACH, RECOVERY, DOCKING };
    enum FLCS_MODE { FLCS_MANUAL, FLCS_AUTO, FLCS_HELM };
    enum TRAN_TYPE {
        TRANSITION_NONE,
        TRANSITION_DROP_CAM,
        TRANSITION_DROP_ORBIT,
        TRANSITION_MAKE_ORBIT,
        TRANSITION_TIME_SKIP,
        TRANSITION_DEATH_SPIRAL,
        TRANSITION_DEAD
    };

    enum FLIGHT_MODEL { FM_STANDARD, FM_RELAXED, FM_ARCADE };
    enum LANDING_MODEL { LM_STANDARD, LM_EASIER };

    // CONSTRUCTORS:
    Ship(const char* ship_name, const char* reg_num, ShipDesign* design, int IFF = 0, int cmd_ai = 0, const int* loadout = 0);
    virtual ~Ship();

    int operator==(const Ship& s) const { return id == s.id; }

    static void       Initialize();
    static void       Close();

    virtual void      ExecFrame(double seconds);
    virtual void      AeroFrame(double seconds);
    virtual void      StatFrame(double seconds);
    virtual void      DockFrame(double seconds);
    virtual void      LinearFrame(double seconds);
    virtual void      ExecSensors(double seconds);

    void              ExecNavFrame(double seconds);
    void              ExecPhysics(double seconds);
    void              ExecThrottle(double seconds);
    void              ExecSystems(double seconds);

    virtual void      Activate(SimScene& scene);
    virtual void      Deactivate(SimScene& scene);
    virtual void      SelectDetail(double seconds);
    virtual void      SetRegion(SimRegion* rgn);

    // NOTE: Bitmap replaced by UTexture2D*
    virtual int       GetTextureList(List<UTexture2D*>& textures);

    // DIRECTION:
    virtual void      SetControls(MotionController* m);
    virtual void      SetNetworkControl(SimDirector* net_ctrl = 0);
    void              SetDirectorInfo(const char* msg) { director_info = msg; }
    const char*       GetDirectorInfo() const { return director_info; }
    void              SetAIMode(int n) { ai_mode = (BYTE)n; }
    int               GetAIMode() const { return (int)ai_mode; }
    void              SetCommandAILevel(int n) { command_ai_level = (BYTE)n; }
    int               GetCommandAILevel() const { return command_ai_level; }
    virtual int       GetFlightPhase() const { return flight_phase; }
    virtual void      SetFlightPhase(OP_MODE phase);
    bool              IsNetObserver() const { return net_observer_mode; }
    void              SetNetObserver(bool n) { net_observer_mode = n; }

    bool              IsInvulnerable() const { return invulnerable; }
    void              SetInvulnerable(bool n) { invulnerable = n; }

    double            GetHelmHeading() const { return helm_heading; }
    double            GetHelmPitch() const { return helm_pitch; }
    void              SetHelmHeading(double h);
    void              SetHelmPitch(double p);
    virtual void      ApplyHelmYaw(double y);
    virtual void      ApplyHelmPitch(double p);
    virtual void      ApplyPitch(double pitch_acc); // override for G limiter

    void              ArcadeStop() { arcade_velocity *= 0; }

    // CAMERA:
    FVector           BridgeLocation() const { return bridge_vec; }
    FVector           ChaseLocation() const { return chase_vec; }
    FVector           TransitionLocation() const { return transition_loc; }

    // FLIGHT DECK:
    Ship* GetController() const;
    int               NumInbound() const;
    int               NumFlightDecks() const;
    FlightDeck* GetFlightDeck(int i = 0) const;
    Ship* GetCarrier() const { return carrier; }
    FlightDeck* GetDock() const { return dock; }
    void              SetCarrier(Ship* c, FlightDeck* d);
    void              Stow();
    InboundSlot* GetInbound() const { return inbound; }
    void              SetInbound(InboundSlot* s);

    // DRIVE SYSTEMS:
    int               GetFuelLevel() const; // (0-100) percent of full tank
    void              SetThrottle(double percent);
    void              SetAugmenter(bool enable);
    double            Thrust(double seconds) const;
    double            VelocityLimit() const { return vlimit; }
    Drive* GetDrive() const { return main_drive; }
    double            Throttle() const { return throttle; }
    bool              Augmenter() const { return augmenter; }
    QuantumDrive* GetQuantumDrive() const { return quantum_drive; }
    Farcaster* GetFarcaster() const { return farcaster; }

    bool              IsAirborne() const;
    bool              IsDropCam() const { return transition_type == TRANSITION_DROP_CAM; }
    bool              IsDropping() const { return transition_type == TRANSITION_DROP_ORBIT; }
    bool              IsAttaining() const { return transition_type == TRANSITION_MAKE_ORBIT; }
    bool              IsSkipping() const { return transition_type == TRANSITION_TIME_SKIP; }
    bool              IsDying() const { return transition_type == TRANSITION_DEATH_SPIRAL; }
    bool              IsDead() const { return transition_type == TRANSITION_DEAD; }
    bool              InTransition() const { return transition_type != TRANSITION_NONE; }
    void              DropOrbit();
    void              MakeOrbit();
    bool              CanTimeSkip();
    bool              IsInCombat();
    void              TimeSkip();
    void              DropCam(double time = 10, double range = 0);
    void              DeathSpiral();
    void              CompleteTransition();
    void              SetTransition(double trans_time, int trans_type, const FVector& trans_loc);

    double            CompassHeading() const;
    double            CompassPitch() const;
    double            AltitudeMSL() const;
    double            AltitudeAGL() const;
    double            GForce() const;

    virtual void      SetupAgility();

    // FLIGHT CONTROL SYSTEM (FLCS):
    void              ExecFLCSFrame();
    void              CycleFLCSMode();
    void              SetFLCSMode(int mode);
    int               GetFLCSMode() const;
    void              SetTransX(double t);
    void              SetTransY(double t);
    void              SetTransZ(double t);

    bool              IsGearDown();
    void              LowerGear();
    void              RaiseGear();
    void              ToggleGear();
    void              ToggleNavlights();

    // WEAPON SYSTEMS:
    virtual void      CheckFriendlyFire();
    virtual void      CheckFire(bool c) { check_fire = c; }
    virtual bool      CheckFire() const { return (check_fire || net_observer_mode) ? true : false; }
    virtual void      SelectWeapon(int n, int w);
    virtual bool      FireWeapon(int n);
    virtual bool      FirePrimary() { return FireWeapon(primary); }
    virtual bool      FireSecondary() { return FireWeapon(secondary); }
    virtual bool      FireDecoy();
    virtual void      CyclePrimary();
    virtual void      CycleSecondary();
    virtual Weapon* GetPrimary() const;
    virtual Weapon* GetSecondary() const;
    virtual Weapon* GetWeaponByIndex(int n);
    virtual WeaponGroup* GetPrimaryGroup() const;
    virtual WeaponGroup* GetSecondaryGroup() const;
    virtual Weapon* GetDecoy() const;
    virtual List<SimShot>& GetActiveDecoys();
    virtual void      AddActiveDecoy(Drone* d);
    virtual int* GetLoadout() { return loadout; }

    List<SimShot>& GetThreatList();
    void              AddThreat(SimShot* s);
    void              DropThreat(SimShot* s);

    virtual bool         Update(SimObject* obj);
    virtual const char* GetObserverName() const { return name; }

    virtual int       GetMissileEta(int index) const;
    virtual void      SetMissileEta(int id, int eta);

    virtual WeaponDesign* GetPrimaryDesign() const;
    virtual WeaponDesign* GetSecondaryDesign() const;

    virtual void      SetTarget(SimObject* t, SimSystem* sub = 0, bool from_net = false);
    virtual SimObject* GetTarget() const { return target; }
    virtual SimSystem* GetSubTarget() const { return subtarget; }
    virtual void      CycleSubTarget(int dir = 1);
    virtual void      DropTarget();
    virtual void      LockTarget(int type = SimObject::SIM_SHIP, bool closest = false, bool hostile = false);
    virtual void      LockTarget(SimObject* candidate);
    virtual bool      IsTracking(SimObject* tgt);
    virtual bool      GetTrigger(int i) const;
    virtual void      SetTrigger(int i);

    Ship* GetWard() const { return ward; }
    void              SetWard(Ship* s);

    // SHIELD SYSTEMS:
    virtual double    InflictDamage(double damage, SimShot* shot = 0, int hit_type = 3, FVector hull_impact = FVector(0, 0, 0));
    virtual double    InflictSystemDamage(double damage, SimShot* shot, FVector impact);

    virtual void      InflictNetDamage(double damage, SimShot* shot = 0);
    virtual void      InflictNetSystemDamage(SimSystem* system, double damage, BYTE type);
    virtual void      SetNetSystemStatus(SimSystem* system, SYSTEM_STATUS status, int power, int reactor, double avail);
    virtual void      SetIntegrity(float n) { integrity = n; }

    virtual void      Destroy();
    virtual int       ShieldStrength() const;
    virtual int       HullStrength() const;
    virtual int       HitBy(SimShot* shot, FVector& impact);
    virtual int       CollidesWith(Physical& o);

    // SENSORS AND VISIBILITY:
    virtual int       GetContactID() const { return contact_id; }
    virtual int       GetIFF() const { return IFF_code; }
    virtual void      SetIFF(int iff);
    virtual FColor    MarkerColor() const;
    static FColor     IFFColor(int iff);
    virtual void      DoEMCON();
    virtual double    PCS() const;
    virtual double    ACS() const;
    int               NumContacts() const;   // actual sensor contacts
    List<SimContact>& ContactList();
    virtual int       GetSensorMode() const;
    virtual void      SetSensorMode(int mode);
    virtual void      LaunchProbe();
    virtual Weapon*    GetProbeLauncher() const { return probe; }
    virtual Drone*     GetProbe() const { return sensor_drone; }
    virtual void       SetProbe(Drone* d);
    virtual int        GetEMCON() const { return emcon; }
    virtual void       SetEMCON(int e, bool from_net = false);
    virtual SimContact* FindContact(SimObject* s) const;
    virtual bool       IsHostileTo(const SimObject* o) const;

    // GENERAL ACCESSORS:
    const char*       Registry() const { return regnum; }
    void              SetName(const char* ident) { strcpy_s(name, ident); }
    const ShipDesign* Design() const { return design; }
    const char*       Abbreviation() const;
    const char*        DesignName() const;
    const char*        DesignFileName() const;
    static const char* GetShipClassName(int c);
    static int         ClassForName(const char* name);
    const char*        GetShipClassName() const;

    // Add this:
    static const char* GetShipClassName(CLASSIFICATION classification) { return GetShipClassName(static_cast<int>(classification)); }
    CLASSIFICATION    Class() const;
    bool              IsGroundUnit() const;
    bool              IsStarship() const;
    bool              IsDropship() const;
    bool              IsStatic() const;
    bool              IsRogue() const;
    void              SetRogue(bool r = true);
    int               GetFriendlyFire() const { return ff_count; }
    void              SetFriendlyFire(int f);
    void              IncFriendlyFire(int f = 1);
    double            Agility() const { return agility; }
    DWORD             MissionClock() const;
    Graphic*          Cockpit() const;
    void              ShowCockpit();
    void              HideCockpit();
    int               Value() const;
    double            AIValue() const;
    static int        Value(int type);

    const Skin*         GetSkin() const { return skin; }
    void                UseSkin(const Skin* s) { skin = s; }
    void                ShowRep();
    void                HideRep();
    void                EnableShadows(bool enable);

    int                 RespawnCount() const { return respawns; }
    void                SetRespawnCount(int r) { respawns = r; }
    const FVector&      RespawnLoc() const { return respawn_loc; }
    void                SetRespawnLoc(const FVector& rl) { respawn_loc = rl; }

    double              WarpFactor() const { return warp_fov; }
    void                SetWarp(double w) { warp_fov = (float)w; }

    void                MatchOrientation(const Ship& s);

    // ORDERS AND NAVIGATION:
    void                    ExecEvalFrame(double seconds);
    void                    SetLaunchPoint(Instruction* pt);
    void                    AddNavPoint(Instruction* pt, Instruction* afterPoint = 0);
    void                    DelNavPoint(Instruction* pt);
    void                    ClearFlightPlan();
    Instruction*            GetNextNavPoint();
    int                     GetNavIndex(const Instruction* n);
    double                  RangeToNavPoint(const Instruction* n);
    void                    SetNavptStatus(Instruction* n, INSTRUCTION_STATUS status);
    List<Instruction>&      GetFlightPlan();
    int                     FlightPlanLength();
    CombatUnit*             GetCombatUnit() const { return combat_unit; }
    SimElement*             GetElement() const { return element; }
    Ship*                   GetLeader() const;
    int                     GetElementIndex() const;
    int                     GetOrigElementIndex() const;
    void                    SetElement(SimElement* e);

    Instruction*            GetRadioOrders() const;
    void                    ClearRadioOrders();
    void                    HandleRadioMessage(RadioMessage* msg);
    bool                    IsAutoNavEngaged();
    void                    SetAutoNav(bool engage = true);
    void                    CommandMode();

    void                    ClearTrack();
    void                    UpdateTrack();
    int                     TrackLength() const { return ntrack; }
    FVector                 TrackPoint(int i) const;

    // DAMAGE CONTROL AND ENGINEERING:
    List<SimSystem>&        RepairQueue() { return repair_queue; }
    double                  RepairSpeed() const;
    int                     RepairTeams() const;
    void                    RepairSystem(SimSystem* sys);
    void                    IncreaseRepairPriority(int task_index);
    void                    DecreaseRepairPriority(int task_index);
    void                    ExecMaintFrame(double seconds);
    bool                    AutoRepair() const { return auto_repair; }
    void                    EnableRepair(bool e) { auto_repair = e; }
    bool                    MasterCaution() const { return master_caution; }
    void                    ClearCaution() { master_caution = 0; }

    // SYSTEM ACCESSORS:
    List<SimSystem>&        GetSystems() { return systems; }
    List<WeaponGroup>&      GetWeapons() { return weapons; }
    List<Drive>&            GetDrives() { return drives; }
    List<Computer>&         GetComputers() { return computers; }
    List<FlightDeck>& FlightDecks() { return flight_decks; }
    List<PowerSource>& Reactors() { return reactors; }
    List<NavLight>& NavLights() { return navlights; }
    Shield* GetShield() { return shield; }
    Solid* GetShieldRep() { return (Solid*)shieldRep; }
    Sensor* GetSensor() { return sensor; }
    NavSystem* GetNavSystem() { return navsys; }
    FlightComputer* GetFLCS() { return flcs; }
    Thruster* GetThruster() { return thruster; }
    Hangar* GetHangar() { return hangar; }
    LandingGear* GetGear() { return gear; }

    SimSystem* GetSystem(int sys_id);

    static int              GetControlModel() { return control_model; }
    static void             SetControlModel(int n) { control_model = n; }
    static int              GetFlightModel() { return flight_model; }
    static void             SetFlightModel(int f) { flight_model = f; }
    static int              GetLandingModel() { return landing_model; }
    static void             SetLandingModel(int f) { landing_model = f; }
    static double           GetFriendlyFireLevel() { return friendly_fire_level; }
    static void             SetFriendlyFireLevel(double f) { friendly_fire_level = f; }

protected:
    int               CheckShotIntersection(SimShot* shot, FVector& ipt, FVector& hpt, Weapon** wep = 0);
    WeaponGroup* FindWeaponGroup(const char* name);

    char              regnum[16];
    ShipDesign*       design;
    ShipKiller*       killer;
    DetailSet         detail;
    int               detail_level;
    Sim* sim;
    double            vlimit;
    double            agility;
    double            throttle;
    double            throttle_request;
    bool              augmenter;
    float             wep_mass;
    float             wep_resist;

    int               IFF_code;
    int               cmd_chain_index;
    int               ff_count;
    OP_MODE           flight_phase;

    SimObject* target;
    SimSystem* subtarget;
    Ship* ward;
    int               check_fire;
    int               primary;
    int               secondary;

    const Skin*         skin;
    Solid*              cockpit;
    Drive*              main_drive;
    QuantumDrive*       quantum_drive;
    Farcaster*          farcaster;
    Shield*             shield;
    ShieldRep*          shieldRep;
    NavSystem*          navsys;
    FlightComputer*     flcs;
    Sensor* sensor;
    LandingGear* gear;
    Thruster* thruster;
    Weapon* decoy;
    Weapon* probe;
    Drone* sensor_drone;
    Hangar* hangar;
    List<SimShot>  decoy_list;
    List<SimShot>  threat_list;

    List<SimSystem>   systems;
    List<PowerSource> reactors;
    List<WeaponGroup> weapons;
    List<Drive>       drives;
    List<Computer>    computers;
    List<FlightDeck>  flight_decks;
    List<NavLight>    navlights;
    List<SimSystem>   repair_queue;

    CombatUnit* combat_unit;
    SimElement* element;
    int               orig_elem_index;
    Instruction* radio_orders;
    Instruction* launch_point;

    FVector           chase_vec;
    FVector           bridge_vec;

    const char* director_info;
    BYTE              ai_mode;
    BYTE              command_ai_level;
    BYTE              flcs_mode;
    bool              net_observer_mode;

    float             pcs;  // passive sensor cross section
    float             acs;  // active  sensor cross section
    BYTE              emcon;
    BYTE              old_emcon;
    bool              invulnerable;

    DWORD             launch_time;
    DWORD             friendly_fire_time;

    Ship* carrier;
    FlightDeck* dock;
    InboundSlot* inbound;

    SimDirector* net_control;

    FVector* track;
    int               ntrack;
    DWORD             track_time;

    float             helm_heading;
    float             helm_pitch;

    float             altitude_agl;
    float             g_force;

    float             warp_fov;

    float             transition_time;
    int               transition_type;
    FVector           transition_loc;
    FVector           respawn_loc;
    int               respawns;

    bool              master_caution;
    bool              auto_repair;
    DWORD             last_repair_time;
    DWORD             last_eval_time;
    DWORD             last_beam_time;
    DWORD             last_bolt_time;

    int               missile_id[4];
    BYTE              missile_eta[4];
    bool              trigger[4];
    int* loadout;

    int               contact_id;

    static int        control_model;
    static int        flight_model;
    static int        landing_model;
    static double     friendly_fire_level;
};
