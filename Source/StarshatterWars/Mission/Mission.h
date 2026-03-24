/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         Mission.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission
    - Mission definition and parsing.
    - MissionElement has been separated into MissionElement.h to keep this header lighter.
    - MissionLoad and MissionShip remain here for now (implementation stays where it is).
*/

#pragma once

#include "Types.h"
#include "Intel.h"
#include "RLoc.h"
#include "SimUniverse.h"
#include "SimScene.h"
#include "Skin.h"
#include "Physical.h"
#include "List.h"
#include "Text.h"

// Unreal math types:
#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // Math

#include "MissionElement.h"

#include "GameStructs.h"

// +--------------------------------------------------------------------+

class Mission;
class MissionLoad;
class MissionEvent;
class MissionShip;

class CombatGroup;
class CombatUnit;

class Ship;
class SimSystem;
class SimElement;
class ShipDesign;
class WeaponDesign;
class StarSystem;
class Instruction;

class Term;
class TermArray;
class TermStruct;

// +--------------------------------------------------------------------+

class Mission
{
public:
    static const char* TYPENAME() { return "Mission"; }

public:
    Mission();  
    Mission(int id, const char* filename = 0, const char* path = 0);
    virtual ~Mission();

    int operator == (const Mission& m) const { return id == m.id; }

    virtual void            Validate();
    virtual bool            Save();
    virtual bool            ParseMission(const char* buffer);
    virtual void            SetPlayer(MissionElement* player_element);
    virtual MissionElement* GetPlayer();

    // accessors/mutators:
    int               GetIdentity()     const { return id; }
    const char*       FileName()        const { return filename; }
    const char*       GetName()         const { return name; }
    const char*       GetDescription()  const { return desc; }
    const char*       GetSituation()    const { return sitrep; }
    const char*       GetObjective()    const { return objective; }
    const char*       GetSystem()       const { return system; }
    const char*       GetScriptName()   const { return script; }
    const char*       GetSubtitles()    const;
    int               GetStart()        const { return start; }
    double            GetStardate()     const { return stardate; }
    int               GetType()         const { return type; }
    const char*       GetTypeName()     const { return GetRoleName(type); }
    int               GetTeam()         const { return team; }
    bool              IsOK()            const { return ok; }
    bool              IsActive()        const { return active; }
    bool              IsComplete()      const { return complete; }

    StarSystem*       GetStarSystem() const { return star_system; }
    List<StarSystem>& GetSystemList() { return system_list; }
    const char*       GetRegion()     const { return region; }

    List<MissionElement>& GetElements() { return elements; }
    virtual MissionElement* FindElement(const char* name);
    virtual void            AddElement(MissionElement* elem);

    List<MissionEvent>& GetEvents() { return events; }
    MissionEvent* FindEvent(int event_type) const;
    virtual void       AddEvent(MissionEvent* event);

    MissionElement* GetTarget() const { return target; }
    MissionElement* GetWard()   const { return ward; }

    void              SetName(const char* n) { name = n; }
    void              SetIdentity(int i) { id = i; }
    void              SetDescription(const char* d) { desc = d; }
    void              SetSituation(const char* sit) { sitrep = sit; }
    void              SetObjective(const char* obj) { objective = obj; }
    void              SetScriptName(const char* scr) { script = scr; }
    void              SetSystem(const char* sys) { system = sys; }
    void              SetStart(int s) { start = s; }
    void              SetEnd(int e) { end = e; }
    void              SetType(int t) { type = t; }
    void              SetTeam(int iff) { team = iff; }
    void              SetStarSystem(StarSystem* s);
    void              SetRegion(const char* rgn) { region = rgn; }
    void              SetOK(bool a) { ok = a; }
    void              SetActive(bool a) { active = a; }
    void              SetComplete(bool c) { complete = c; }
    void              SetTarget(MissionElement* t) { target = t; }
    void              SetWard(MissionElement* w) { ward = w; }

    void              ClearSystemList();
    void              InitializeFromInfo(const MissionInfo& Info);

    void              IncreaseElemPriority(int index);
    void              DecreaseElemPriority(int index);
    void              IncreaseEventPriority(int index);
    void              DecreaseEventPriority(int index);

    static const char* GetRoleName(int role);
    static EMISSIONTYPE EnumFromName(const char* n);
    static int         TypeFromName(const char* n);

    Text              ErrorMessage() const { return errmsg; }
    void              AddError(Text err);

    Text              Serialize(const char* player_elem = 0, int player_index = 0);

    template <typename TMissionData>
    bool LoadMissionCommon(const TMissionData& InData, bool bFullReset);

    bool LoadFromCampaignMissionData(const FS_CampaignMission& InMission);
    bool LoadFromTemplateMissionData(const FS_TemplateMission& InTemplate);

protected:
    MissionElement* ParseElement(TermStruct* val);
    MissionEvent* ParseEvent(TermStruct* val);
    MissionShip* ParseShip(TermStruct* val, MissionElement* element);
    Instruction* ParseInstruction(TermStruct* val, MissionElement* element);
    void              ParseLoadout(TermStruct* val, MissionElement* element);
    RLoc* ParseRLoc(TermStruct* val);

    int               id;
    char              filename[64];
    char              path[64];
    Text              region;
    Text              system;
    Text              name;
    Text              desc;
    Text              script;
    int               type;
    int               team;
    int               start;
    int               end;
    double            stardate;
    bool              ok;
    bool              active;
    bool              complete;
    bool              degrees;
    Text              objective;
    Text              sitrep;
    Text              errmsg;
    Text              subtitles;
    StarSystem*       star_system;
    List<StarSystem>  system_list;

    List<MissionElement> elements;
    List<MissionEvent>   events;

    MissionElement* target;
    MissionElement* ward;
    MissionElement* current;
};

// +--------------------------------------------------------------------+

class MissionLoad
{
    friend class Mission;

public:
    static const char* TYPENAME() { return "MissionLoad"; }

    MissionLoad(int ship = -1, const char* name = 0);
    ~MissionLoad();

    int               GetShip() const;
    void              SetShip(int ship);

    Text              GetName() const;
    void              SetName(Text name);

    int*              GetStations();
    int               GetStation(int index);
    void              SetStation(int index, int selection);

protected:
    int               ship;
    Text              name;
    int               load[16];
};

// +--------------------------------------------------------------------+

class MissionShip
{
    friend class Mission;

public:
    static const char* TYPENAME() { return "MissionShip"; }

    MissionShip();
    ~MissionShip() {}

    const Text& Name()      const { return name; }
    const Text& RegNum()    const { return regnum; }
    const Text& Region()    const { return region; }
    const Skin* GetSkin()   const { return skin; }

    const FVector& Location()  const { return loc; }
    const FVector& Velocity()  const { return velocity; }

    int               Respawns()  const { return respawns; }
    double            Heading()   const { return heading; }
    double            Integrity() const { return integrity; }
    int               Decoys()    const { return decoys; }
    int               Probes()    const { return probes; }
    const int* Ammo()      const { return ammo; }
    const int* Fuel()      const { return fuel; }

    void              SetName(const char* n) { name = n; }
    void              SetRegNum(const char* n) { regnum = n; }
    void              SetRegion(const char* n) { region = n; }
    void              SetSkin(const Skin* s) { skin = s; }

    void              SetLocation(const FVector& p) { loc = p; }
    void              SetVelocity(const FVector& p) { velocity = p; }

    void              SetRespawns(int r) { respawns = r; }
    void              SetHeading(double h) { heading = h; }
    void              SetIntegrity(double n) { integrity = n; }
    void              SetDecoys(int d) { decoys = d; }
    void              SetProbes(int p) { probes = p; }
    void              SetAmmo(const int* a);
    void              SetFuel(const int* f);

protected:
    Text              name;
    Text              regnum;
    Text              region;
    const Skin* skin;

    FVector           loc;
    FVector           velocity;

    int               respawns;
    double            heading;
    double            integrity;
    int               decoys;
    int               probes;
    int               ammo[16];
    int               fuel[4];
};
