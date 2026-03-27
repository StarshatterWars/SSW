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

#include "Math/Vector.h"   // FVector
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

    int operator == (const CombatUnit& u) const { return this == &u; }

    const char* GetDescription()           const;

    int            GetValue()                    const;
    int            GetSingleValue()              const;
    bool           CanDefend(CombatUnit* unit)   const;
    bool           CanLaunch()                   const;
    double         PowerVersus(CombatUnit* tgt)  const;
    int            AssignMission();
    void           CompleteMission();

    double         MaxRange()                    const;
    double         MaxEffectiveRange()           const;
    double         OptimumRange()                const;

    void           Engage(CombatUnit* tgt);
    void           Disengage();

    // accessors and mutators:
    const Text& GetName()                     const { return name; }
    const Text& GetRegistryNumber()           const { return regnum; }
    const Text& GetDesignName()               const { return design_name; }

    const Text&     GetSkin()                     const { return skin; }
    void            SetSkin(const char* s) { skin = s; }

    int            Type()                        const { return type; }
    int            Count()                       const { return count; }
    int            LiveCount()                   const { return count - dead_count; }
    int            DeadCount()                   const { return dead_count; }
    void           SetDeadCount(int n) { dead_count = n; }
    int            Kill(int n);

    int            Available()                   const { return available; }
    int            GetIFF()                      const { return iff; }

    bool           IsLeader()                    const { return leader; }
    void           SetLeader(bool l) { leader = l; }

    FVector        Location()                    const { return location; }
    void           MoveTo(const FVector& loc);

    Text           GetRegion()                   const { return region; }
    void           SetRegion(Text rgn) { region = rgn; }

    CombatGroup* GetCombatGroup()              const { return group; }
    void           SetCombatGroup(CombatGroup* g) { group = g; }

    FColor         MarkerColor()                 const;
    bool           IsGroundUnit()                const;
    bool           IsStarship()                  const;
    bool           IsDropship()                  const;
    bool           IsStatic()                    const;

    CombatUnit* GetCarrier()                  const { return carrier; }
    void           SetCarrier(CombatUnit* c) { carrier = c; }

    const ShipDesign* GetDesign();
    int            GetShipClass()                const;

    List<CombatUnit>& GetAttackers() { return attackers; }

    double         GetPlanValue()                const { return plan_value; }
    void           SetPlanValue(int v) { plan_value = v; }

    double         GetSustainedDamage()          const { return sustained_damage; }
    void           SetSustainedDamage(double d) { sustained_damage = d; }

    double         GetHeading()                  const { return heading; }
    void           SetHeading(double d) { heading = d; }

    double         GetNextJumpTime()             const { return jump_time; }

private:
    Text                 name;
    Text                 regnum;
    Text                 design_name;
    Text                 skin;

    int                  type;
    const ShipDesign* design;

    int                  count;
    int                  dead_count;
    int                  available;

    int                  iff;
    bool                 leader;

    Text                 region;

    FVector              location;        // TEMPLATE: was Point

    double               plan_value;      // scratch pad for plan modules
    double               launch_time;
    double               jump_time;
    double               sustained_damage;
    double               heading;

    CombatUnit* carrier;
    List<CombatUnit>     attackers;
    CombatUnit* target;
    CombatGroup* group;

    private:
        Text resolved_design_display_name;
        Text resolved_design_abrv;
        Text resolved_design_class;
        int  resolved_design_type = 0;
        bool b_has_resolved_design = false;

public:
    void SetResolvedDesignData(const char* InDisplayName,
        const char* InAbrv,
        const char* InClass,
        int InType)
    {
        resolved_design_display_name = InDisplayName ? InDisplayName : "";
        resolved_design_abrv = InAbrv ? InAbrv : "";
        resolved_design_class = InClass ? InClass : "";
        resolved_design_type = InType;
        b_has_resolved_design = true;
    }

    bool HasResolvedDesignData() const { return b_has_resolved_design; }

    const Text& ResolvedDisplayName() const { return resolved_design_display_name; }
    const Text& ResolvedAbrv() const { return resolved_design_abrv; }
    const Text& ResolvedClass() const { return resolved_design_class; }
    int ResolvedType() const { return resolved_design_type; }
};
