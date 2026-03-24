/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionElement (split from Mission.h)
    - Mission definition element (NOT a SimElement).
    - API kept compatible with legacy callers (CampaignMissionStarship, dialogs).
*/

#pragma once

#include "Types.h"
#include "RLoc.h"
#include "List.h"
#include "Text.h"

#include "Math/Vector.h"
#include "Math/Color.h"

// Forward decls:
class Instruction;
class MissionLoad;
class MissionShip;
class CombatGroup;
class CombatUnit;
class ShipDesign;
class Skin;

class MissionElement
{
    friend class Mission;

public:
    static const char* TYPENAME() { return "MissionElement"; }

    MissionElement();
    ~MissionElement();

    int operator==(const MissionElement& r) const { return id == r.id; }

    // ------------------------------------------------------------
    // Identity / labels (legacy noun-style accessors)
    // ------------------------------------------------------------
    int         Identity()      const { return id; }
    const Text& Name()          const { return name; }
    Text        Abbreviation()  const;
    const Text& Carrier()       const { return carrier; }
    const Text& Commander()     const { return commander; }
    const Text& Squadron()      const { return squadron; }
    const Text& Path()          const { return path; }
    int         ElementID()     const { return elem_id; }

    // ------------------------------------------------------------
    // Design / skin / counts
    // ------------------------------------------------------------
    const ShipDesign* GetDesign()      const { return design; }
    const Skin* GetSkin()        const { return skin; }
    int               Count()          const { return count; }
    int               MaintCount()     const { return maint_count; }
    int               DeadCount()      const { return dead_count; }

    // ------------------------------------------------------------
    // Team / role / flags
    // ------------------------------------------------------------
    int  GetIFF()         const { return IFF_code; }
    int  IntelLevel()     const { return intel; }
    int  MissionRole()    const { return mission_role; }

    // bool-style naming:
    bool IsPlayer()       const { return player != 0; }
    bool IsAlert()        const { return alert; }
    bool IsPlayable()     const { return playable; }
    bool IsRogue()        const { return rogue; }
    bool IsInvulnerable() const { return invulnerable; }

    // legacy “what kind of element is it” queries:
    Text RoleName()       const;
    FColor MarkerColor()  const;
    bool  IsStarship()    const;
    bool  IsDropship()    const;
    bool  IsStatic()      const;
    bool  IsGroundUnit()  const;
    bool  IsSquadron()    const;
    bool  IsCarrier()     const;

    // ------------------------------------------------------------
    // AI / mission behavior (proper Get/Set)
    // ------------------------------------------------------------
    int  GetCommandAI()   const { return command_ai; }
    void SetCommandAI(int a) { command_ai = a; }

    int  GetZoneLock()    const { return zone_lock; }
    void SetZoneLock(int z) { zone_lock = z; }

    int  RespawnCount()   const { return respawns; }
    int  HoldTime()       const { return hold_time; }

    // ------------------------------------------------------------
    // Region / location / heading
    // ------------------------------------------------------------
    const Text& Region()   const { return rgn_name; }
    FVector     Location() const;
    RLoc& GetRLoc() { return rloc; }
    double      Heading()  const { return heading; }

    Text        GetShipName(int n) const;
    Text        GetRegistry(int n) const;

    // ------------------------------------------------------------
    // Lists (legacy container API)
    // ------------------------------------------------------------
    List<Instruction>& Objectives() { return objectives; }
    List<Text>& Instructions() { return instructions; }
    List<Instruction>& NavList() { return navlist; }
    List<MissionLoad>& Loadouts() { return loadouts; }
    List<MissionShip>& Ships() { return ships; }

    // Compatibility wrappers for refactored call sites:
    List<MissionLoad>& GetLoadouts() { return loadouts; }   // matches your error sites

    // ------------------------------------------------------------
    // Mutators (legacy naming)
    // ------------------------------------------------------------
    void SetName(const char* n) { name = n; }
    void SetCarrier(const char* c) { carrier = c; }
    void SetCommander(const char* c) { commander = c; }
    void SetSquadron(const char* s) { squadron = s; }
    void SetPath(const char* p) { path = p; }
    void SetElementID(int elementID) { elem_id = elementID; }
    void SetDesign(const ShipDesign* d) { design = d; }
    void SetSkin(const Skin* s) { skin = s; }
    void SetCount(int n) { count = n; }
    void SetMaintCount(int n) { maint_count = n; }
    void SetDeadCount(int n) { dead_count = n; }
    void SetIFF(int iff) { IFF_code = iff; }
    void SetIntelLevel(int i) { intel = i; }
    void SetMissionRole(int r) { mission_role = r; }
    void SetPlayer(int p) { player = p; }         // int for legacy data
    void SetPlayable(bool p) { playable = p; }
    void SetRogue(bool r) { rogue = r; }
    void SetInvulnerable(bool n) { invulnerable = n; }
    void SetAlert(bool a) { alert = a; }
    void SetRegion(const char* rgn) { rgn_name = rgn; }

    void SetLocation(const FVector& p);
    void SetRLoc(const RLoc& r);
    void SetHeading(double h) { heading = h; }
    void SetRespawnCount(int r) { respawns = r; }
    void SetHoldTime(int t) { hold_time = t; }

    // ------------------------------------------------------------
    // Missing methods causing your compile errors
    // ------------------------------------------------------------
    void AddObjective(Instruction* obj) { objectives.append(obj); }

    void AddNavPoint(Instruction* pt, Instruction* afterPoint = 0);
    void DelNavPoint(Instruction* pt);
    void ClearFlightPlan();
    int  GetNavIndex(const Instruction* n);

    void AddInstruction(const char* i) { instructions.append(new Text(i)); }

    // ------------------------------------------------------------
    // Combat links
    // ------------------------------------------------------------
    CombatGroup* GetCombatGroup() { return combat_group; }
    void         SetCombatGroup(CombatGroup* g) { combat_group = g; }

    CombatUnit* GetCombatUnit() { return combat_unit; }
    void         SetCombatUnit(CombatUnit* u) { combat_unit = u; }

protected:
    int               id = 0;
    Text              name;
    Text              carrier;
    Text              commander;
    Text              squadron;
    Text              path;
    int               elem_id = 0;

    const ShipDesign* design = nullptr;
    const Skin* skin = nullptr;

    int               count = 0;
    int               maint_count = 0;
    int               dead_count = 0;

    int               IFF_code = 0;
    int               mission_role = 0;
    int               intel = 0;

    int               respawns = 0;
    int               hold_time = 0;
    int               zone_lock = 0;
    int               player = 0;
    int               command_ai = 0;

    bool              alert = false;
    bool              playable = false;
    bool              rogue = false;
    bool              invulnerable = false;

    Text              rgn_name;
    RLoc              rloc;
    double            heading = 0.0;

    CombatGroup* combat_group = nullptr;
    CombatUnit* combat_unit = nullptr;

    List<Instruction> objectives;
    List<Text>        instructions;
    List<Instruction> navlist;
    List<MissionLoad> loadouts;
    List<MissionShip> ships;
};
