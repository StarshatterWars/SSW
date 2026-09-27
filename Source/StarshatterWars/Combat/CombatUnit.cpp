/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    GAME
    FILE:         CombatUnit.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    A ship, station, or ground unit in the campaign system.
*/

#include "CombatUnit.h"
#include "CombatGroup.h"
#include "Campaign.h"
#include "ShipDesign.h"
#include "Ship.h"
#include "GameStructs.h"
#include "Game.h"

inline double random() { return (double)rand() / (double)RAND_MAX; }

// +----------------------------------------------------------------------+

CombatUnit::CombatUnit(const char* n, const char* reg, int t, const char* d, int c, int i)
    : name(n)
    , regnum(reg)
    , design_name(d)
    , skin("")
    , type(t)
    , design(0)
    , count(c)
    , dead_count(0)
    , available(c)
    , iff(i)
    , leader(false)
    , region("")
    , location(FVector::ZeroVector)
    , plan_value(0)
    , launch_time(-1e6)
    , jump_time(0)
    , sustained_damage(0)
    , heading(0)
    , carrier(0)
    , attackers()
    , target(0)
    , group(0)
    , resolved_design_display_name("")
    , resolved_design_abrv("")
    , resolved_design_class("")
    , resolved_design_type(0)
    , b_has_resolved_design(false)
    , resolved_mass(0.0)
    , resolved_scale(0.0)
    , resolved_vlimit(0.0)
    , resolved_agility(0.0)
    , resolved_detect(0.0)
    , resolved_repair_teams(0)
    , resolved_description("")
    , resolved_weapon_summary("")
    , b_has_resolved_stats(false)
    , b_has_resolved_desc(false)
    , b_has_resolved_weapons(false)
{
}

CombatUnit::CombatUnit(const CombatUnit& u)
    : name(u.name)
    , regnum(u.regnum)
    , design_name(u.design_name)
    , skin(u.skin)
    , type(u.type)
    , design(u.design)
    , count(u.count)
    , dead_count(u.dead_count)
    , available(u.available)
    , iff(u.iff)
    , leader(u.leader)
    , region(u.region)
    , location(u.location)
    , plan_value(u.plan_value)
    , launch_time(u.launch_time)
    , jump_time(u.jump_time)
    , sustained_damage(u.sustained_damage)
    , heading(u.heading)
    , carrier(u.carrier)
    , attackers()
    , target(0)
    , group(0)
    , resolved_design_display_name(u.resolved_design_display_name)
    , resolved_design_abrv(u.resolved_design_abrv)
    , resolved_design_class(u.resolved_design_class)
    , resolved_design_type(u.resolved_design_type)
    , b_has_resolved_design(u.b_has_resolved_design)
    , resolved_mass(u.resolved_mass)
    , resolved_scale(u.resolved_scale)
    , resolved_vlimit(u.resolved_vlimit)
    , resolved_agility(u.resolved_agility)
    , resolved_detect(u.resolved_detect)
    , resolved_repair_teams(u.resolved_repair_teams)
    , resolved_description(u.resolved_description)
    , resolved_weapon_summary(u.resolved_weapon_summary)
    , b_has_resolved_stats(u.b_has_resolved_stats)
    , b_has_resolved_desc(u.b_has_resolved_desc)
    , b_has_resolved_weapons(u.b_has_resolved_weapons)
{
}

CombatUnit& CombatUnit::operator=(const CombatUnit& u)
{
    if (this == &u)
        return *this;

    name = u.name;
    regnum = u.regnum;
    design_name = u.design_name;
    skin = u.skin;

    type = u.type;
    design = u.design;

    count = u.count;
    dead_count = u.dead_count;
    available = u.available;

    iff = u.iff;
    leader = u.leader;

    region = u.region;
    location = u.location;

    plan_value = u.plan_value;
    launch_time = u.launch_time;
    jump_time = u.jump_time;
    sustained_damage = u.sustained_damage;
    heading = u.heading;

    carrier = u.carrier;
    attackers.clear();
    target = 0;
    group = 0;

    resolved_design_display_name = u.resolved_design_display_name;
    resolved_design_abrv = u.resolved_design_abrv;
    resolved_design_class = u.resolved_design_class;
    resolved_design_type = u.resolved_design_type;
    b_has_resolved_design = u.b_has_resolved_design;

    resolved_mass = u.resolved_mass;
    resolved_scale = u.resolved_scale;
    resolved_vlimit = u.resolved_vlimit;
    resolved_agility = u.resolved_agility;
    resolved_detect = u.resolved_detect;
    resolved_repair_teams = u.resolved_repair_teams;

    resolved_description = u.resolved_description;
    resolved_weapon_summary = u.resolved_weapon_summary;

    b_has_resolved_stats = u.b_has_resolved_stats;
    b_has_resolved_desc = u.b_has_resolved_desc;
    b_has_resolved_weapons = u.b_has_resolved_weapons;

    return *this;
}

void CombatUnit::SetResolvedDesignData(
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
    const char* InWeaponSummary)
{
    resolved_design_display_name = InDisplayName ? InDisplayName : "";
    resolved_design_abrv = InAbrv ? InAbrv : "";
    resolved_design_class = InClass ? InClass : "";
    resolved_design_type = InType;

    resolved_mass = InMass;
    resolved_scale = InScale;
    resolved_vlimit = InVLimit;
    resolved_agility = InAgility;
    resolved_detect = InDetect;
    resolved_repair_teams = InRepairTeams;

    resolved_description = InDescription ? InDescription : "";
    resolved_weapon_summary = InWeaponSummary ? InWeaponSummary : "";

    b_has_resolved_design = true;
    b_has_resolved_stats = true;
    b_has_resolved_desc = resolved_description.length() > 0;
    b_has_resolved_weapons = resolved_weapon_summary.length() > 0;

    UE_LOG(LogTemp, Warning,
        TEXT("[CombatUnit] Stored: class='%s' mass=%.0f detect=%.0f repair=%d"),
        ANSI_TO_TCHAR(resolved_design_class.data()),
        resolved_mass,
        resolved_detect,
        resolved_repair_teams);
}

// +----------------------------------------------------------------------+

const ShipDesign* CombatUnit::GetDesign()
{
    if (!design)
        design = ShipDesign::GetDesignName(design_name);

    return design;
}

int CombatUnit::GetShipClass() const
{
    if (design)
        return design->type;

    return type;
}

int CombatUnit::GetValue() const
{
    return GetSingleValue() * LiveCount();
}

int CombatUnit::GetSingleValue() const
{
    return Ship::Value(GetShipClass());
}

// +----------------------------------------------------------------------+

const char* CombatUnit::GetDescription() const
{
    if (!design) {
        CombatUnit* pThis = (CombatUnit*)this;
        pThis->GetDesign();
    }

    static char desc[256];

    if (!design) {
        strcpy_s(desc, "[unknown]");
    }
    else if (count > 1) {
        sprintf_s(desc, "%dx %s %s", LiveCount(), design->abrv, design->DisplayName());
    }
    else {
        if (regnum.length() > 0)
            sprintf_s(desc, "%s-%s %s", design->abrv, (const char*)regnum, (const char*)name);
        else
            sprintf_s(desc, "%s %s", design->abrv, (const char*)name);

        if (dead_count > 0) {
            strcat_s(desc, " ");
            strcat_s(desc, "[killed in action]");
        }
    }

    return desc;
}

// +----------------------------------------------------------------------+

bool CombatUnit::CanLaunch() const
{
    bool result = false;

    switch (type) {
    case (int)CLASSIFICATION::FIGHTER:
    case (int)CLASSIFICATION::ATTACK:
        result = (Campaign::GetStardate() - launch_time) >= 300;
        break;

    case (int)CLASSIFICATION::CORVETTE:
    case (int)CLASSIFICATION::FRIGATE:
    case (int)CLASSIFICATION::DESTROYER:
    case (int)CLASSIFICATION::CRUISER:
    case (int)CLASSIFICATION::CARRIER:
        result = true;
        break;
    }

    return result;
}

// +----------------------------------------------------------------------+

FColor CombatUnit::MarkerColor() const
{
    return Ship::IFFColor(iff);
}

bool CombatUnit::IsGroundUnit() const
{
    return (design && (design->type & (int)CLASSIFICATION::GROUND_UNITS)) ? true : false;
}

bool CombatUnit::IsStarship() const
{
    return (design && (design->type & (int)CLASSIFICATION::STARSHIPS)) ? true : false;
}

bool CombatUnit::IsDropship() const
{
    return (design && (design->type & (int)CLASSIFICATION::DROPSHIPS)) ? true : false;
}

bool CombatUnit::IsStatic() const
{
    return design && (design->type >= (int)CLASSIFICATION::STATION);
}

// +----------------------------------------------------------------------+

double CombatUnit::MaxRange() const
{
    return 100e3;
}

double CombatUnit::MaxEffectiveRange() const
{
    return 50e3;
}

double CombatUnit::OptimumRange() const
{
    if (type == (int)CLASSIFICATION::FIGHTER || type == (int)CLASSIFICATION::ATTACK)
        return 15e3;

    return 30e3;
}

// +----------------------------------------------------------------------+

bool CombatUnit::CanDefend(CombatUnit* unit) const
{
    if (unit == 0 || unit == this)
        return false;

    if (type > (int)CLASSIFICATION::STATION)
        return false;

    const double distance = (location - unit->location).Size();

    if (type > unit->type)
        return false;

    if (distance > MaxRange())
        return false;

    return true;
}

// +----------------------------------------------------------------------+

double CombatUnit::PowerVersus(CombatUnit* tgt) const
{
    if (tgt == 0 || tgt == this || available < 1)
        return 0;

    if (type > (int)CLASSIFICATION::STATION)
        return 0;

    double effectiveness = 1;
    const double distance = (location - tgt->location).Size();

    if (distance > MaxRange())
        return 0;

    if (distance > MaxEffectiveRange())
        effectiveness = 0.5;

    if (type == (int)CLASSIFICATION::FIGHTER) {
        if (tgt->type == (int)CLASSIFICATION::FIGHTER || tgt->type == (int)CLASSIFICATION::ATTACK)
            return (int)CLASSIFICATION::FIGHTER * 2 * available * effectiveness;
        else
            return 0;
    }
    else if (type == (int)CLASSIFICATION::ATTACK) {
        if (tgt->type > (int)CLASSIFICATION::ATTACK)
            return (int)CLASSIFICATION::ATTACK * 3 * available * effectiveness;
        else
            return 0;
    }
    else if (type == (int)CLASSIFICATION::CARRIER) {
        return 0;
    }
    else if (type == (int)CLASSIFICATION::SWACS) {
        return 0;
    }
    else if (type == (int)CLASSIFICATION::CRUISER) {
        if (tgt->type <= (int)CLASSIFICATION::ATTACK)
            return type * effectiveness;
        else
            return 0;
    }
    else {
        if (tgt->type > (int)CLASSIFICATION::ATTACK)
            return type * effectiveness;
        else
            return type * 0.1 * effectiveness;
    }
}

// +----------------------------------------------------------------------+

int CombatUnit::AssignMission()
{
    int assign = count;

    if (count > 4)
        assign = 4;

    if (assign > 0) {
        available -= assign;
        launch_time = Campaign::GetStardate();
        return assign;
    }

    return 0;
}

// +----------------------------------------------------------------------+

void CombatUnit::CompleteMission()
{
    Disengage();

    if (count > 4)
        available += 4;
    else
        available += count;
}

// +----------------------------------------------------------------------+

void CombatUnit::MoveTo(const FVector& loc)
{
    if (!carrier)
        location = loc;
    else
        location = carrier->location;
}

// +----------------------------------------------------------------------+

void CombatUnit::Engage(CombatUnit* tgt)
{
    if (!tgt)
        Disengage();
    else if (!tgt->attackers.contains(this))
        tgt->attackers.append(this);

    target = tgt;
}

void CombatUnit::Disengage()
{
    if (target)
        target->attackers.remove(this);

    target = 0;
}

// +----------------------------------------------------------------------+

static int KillGroup(CombatGroup* group)
{
    int value_killed = 0;

    if (group) {
        ListIter<CombatUnit> u_iter = group->GetUnits();
        while (++u_iter) {
            CombatUnit* u = u_iter.value();
            value_killed += u->GetKill(u->LiveCount());
        }

        ListIter<CombatGroup> g_iter = group->GetComponents();
        while (++g_iter) {
            CombatGroup* g = g_iter.value();
            value_killed += KillGroup(g);
        }
    }

    return value_killed;
}

int CombatUnit::GetKill(int n)
{
    int killed = n;

    if (killed > LiveCount())
        killed = LiveCount();

    dead_count += killed;

    int value_killed = killed * GetSingleValue();

    if (killed) {
        if (type == (int)CLASSIFICATION::CARRIER ||
            type == (int)CLASSIFICATION::STATION ||
            type == (int)CLASSIFICATION::STARBASE) {

            if (group) {
                ListIter<CombatGroup> iter = group->GetComponents();
                while (++iter) {
                    CombatGroup* g = iter.value();
                    value_killed += KillGroup(g);
                }
            }
        }
    }

    return value_killed;
}