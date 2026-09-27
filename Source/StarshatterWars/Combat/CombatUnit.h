#pragma once

/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    GAME
    FILE:         CombatUnit.h
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    A ship, station, or ground unit in the campaign system.
*/

#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "List.h"
#include "GameStructs.h"

#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class CombatGroup;
class ShipDesign;

// +--------------------------------------------------------------------+

class CombatUnit
{
public:
    static const char* TYPENAME() { return "CombatUnit"; }

    CombatUnit(const char* n, const char* reg, int t, const char* dname, int number, int i);
    CombatUnit(const CombatUnit& unit);
    CombatUnit& operator=(const CombatUnit& unit);

    int operator == (const CombatUnit& u) const { return this == &u; }

    const char* GetDescription() const;

    int    GetValue() const;
    int    GetSingleValue() const;
    bool   CanDefend(CombatUnit* unit) const;
    bool   CanLaunch() const;
    double PowerVersus(CombatUnit* tgt) const;
    int    AssignMission();
    void   CompleteMission();

    double MaxRange() const;
    double MaxEffectiveRange() const;
    double OptimumRange() const;

    void   Engage(CombatUnit* tgt);
    void   Disengage();

    const Text& GetName() const { return name; }
    const Text& GetRegistryNumber() const { return regnum; }
    const Text& GetDesignName() const { return design_name; }

    const Text& GetSkin() const { return skin; }
    void SetSkin(const char* s) { skin = s; }

    int  GetType() const { return type; }
    int  GetCount() const { return count; }
    int  LiveCount() const { return count - dead_count; }
    int  DeadCount() const { return dead_count; }
    void SetDeadCount(int n) { dead_count = n; }
    int  GetKill(int n);

    int  Available() const { return available; }
    int  GetIFF() const { return iff; }

    bool IsLeader() const { return leader; }
    void SetLeader(bool l) { leader = l; }

    FVector GetLocation() const { return location; }
    void MoveTo(const FVector& loc);

    Text GetRegion() const { return region; }
    void SetRegion(Text rgn) { region = rgn; }

    CombatGroup* GetCombatGroup() const { return group; }
    void SetCombatGroup(CombatGroup* g) { group = g; }

    FColor MarkerColor() const;
    bool   IsGroundUnit() const;
    bool   IsStarship() const;
    bool   IsDropship() const;
    bool   IsStatic() const;

    CombatUnit* GetCarrier() const { return carrier; }
    void SetCarrier(CombatUnit* c) { carrier = c; }

    const ShipDesign* GetDesign();
    int GetShipClass() const;

    List<CombatUnit>& GetAttackers() { return attackers; }

    double GetPlanValue() const { return plan_value; }
    void SetPlanValue(int v) { plan_value = v; }

    double GetSustainedDamage() const { return sustained_damage; }
    void SetSustainedDamage(double d) { sustained_damage = d; }

    double GetHeading() const { return heading; }
    void SetHeading(double d) { heading = d; }

    double GetNextJumpTime() const { return jump_time; }

    void SetResolvedDesignData(
        const char* InDisplayName,
        const char* InAbrv,
        const char* InClass,
        int InType,
        double InMass,
        double InScale,
        double InVLimit,
        double InAgility,
        double InDetect,
        int InRepairTeams,
        const char* InDescription,
        const char* InWeaponSummary);

    bool HasResolvedDesignData() const { return b_has_resolved_design; }

    const Text& ResolvedDisplayName() const { return resolved_design_display_name; }
    const Text& ResolvedAbrv() const { return resolved_design_abrv; }
    const Text& ResolvedClass() const { return resolved_design_class; }
    int ResolvedType() const { return resolved_design_type; }

    bool HasResolvedDesignStats() const { return b_has_resolved_stats; }
    bool HasResolvedDescription() const { return b_has_resolved_desc; }
    bool HasResolvedWeaponSummary() const { return b_has_resolved_weapons; }

    double ResolvedMass() const { return resolved_mass; }
    double ResolvedScale() const { return resolved_scale; }
    double ResolvedVLimit() const { return resolved_vlimit; }
    double ResolvedAgility() const { return resolved_agility; }
    double ResolvedDetect() const { return resolved_detect; }
    int    ResolvedRepairTeams() const { return resolved_repair_teams; }

    const Text& ResolvedDescription() const { return resolved_description; }
    const Text& ResolvedWeaponSummary() const { return resolved_weapon_summary; }

private:
    Text name;
    Text regnum;
    Text design_name;
    Text skin;

    int type;
    const ShipDesign* design;

    int count;
    int dead_count;
    int available;

    int  iff;
    bool leader;

    Text region;

    FVector location;

    double plan_value;
    double launch_time;
    double jump_time;
    double sustained_damage;
    double heading;

    CombatUnit* carrier;
    List<CombatUnit> attackers;
    CombatUnit* target;
    CombatGroup* group;

    Text resolved_design_display_name;
    Text resolved_design_abrv;
    Text resolved_design_class;
    int  resolved_design_type = 0;
    bool b_has_resolved_design = false;

    double resolved_mass = 0.0;
    double resolved_scale = 0.0;
    double resolved_vlimit = 0.0;
    double resolved_agility = 0.0;
    double resolved_detect = 0.0;
    int    resolved_repair_teams = 0;

    Text resolved_description;
    Text resolved_weapon_summary;

    bool b_has_resolved_stats = false;
    bool b_has_resolved_desc = false;
    bool b_has_resolved_weapons = false;
};