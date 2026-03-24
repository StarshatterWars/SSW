/*  Project Starshatter 4.5
    Fractal Dev Studios
    Copyright © 2025.

    SUBSYSTEM:    Stars.exe
    FILE:         CombatAction.h
    AUTHOR:       Carlos Bott

    UNREAL PORT:
    - Maintains all variables and methods (names, signatures, members).
    - Uses UE-compatible shims for Text, List, Point (Geometry.h).
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "List.h"
#include "GameStructs.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"


// +--------------------------------------------------------------------+

class Combatant;
class CombatAction;
class CombatActionReq;
class Campaign;

// +--------------------------------------------------------------------+

class CombatAction
{
public:
    static const char* TYPENAME() { return "CombatAction"; }


    CombatAction(int id, int type, int subtype, int team);
    ~CombatAction();

    int  operator==(const CombatAction& a) const { return id == a.id; }

    static const int32 TIME_NEVER = (int32)1e9;

    bool IsAvailable() const;
    void FireAction();
    void FailAction();

    void AddRequirement(int action, int stat, bool notx = false);
    void AddRequirement(Combatant* c1, Combatant* c2, int comp, int score);
    void AddRequirement(Combatant* c1, ECOMBATGROUP_TYPE group_type, int group_id, int comp, int score, int intel = 0);

    static int TypeFromName(const char* n);
    static int StatusFromName(const char* n);

    // accessors/mutators:
    int         Identity()     const { return id; }
    int         Type()         const { return type; }
    int         Subtype()      const { return subtype; }
    int         OpposingType() const { return opp_type; }
    int         GetIFF()       const { return team; }
    int         Status()       const { return status; }
    ECombatEventSource  GetSource()   const { return source; }
    FVector     Location()     const { return loc; }
    const char* GetTitle() const;
    const char* GetSoundFile() const;
    const char* System()       const { return system.data(); }
    const char* Region()       const { return region.data(); }
    const char* Filename()     const { return text_file.data(); }
    const char* ImageFile()    const { return image_file.data(); }
    const char* SceneFile()    const { return scene_file.data(); }
    int         Count()        const { return count; }
    int         ExecTime()     const { return time; }
    int         StartBefore()  const { return start_before; }
    int         StartAfter()   const { return start_after; }
    int         MinRank()      const { return min_rank; }
    int         MaxRank()      const { return max_rank; }
    int         Delay()        const { return delay; }
    int         Probability()  const { return probability; }
    int         AssetType()    const { return asset_type; }
    int         AssetId()      const { return asset_id; }
    List<Text>& AssetKills() { return asset_kills; }
    int         TargetType()   const { return target_type; }
    int         TargetId()     const { return target_id; }
    int         TargetIFF()    const { return target_iff; }
    List<Text>& TargetKills() { return target_kills; }
    const char* GetText()      const { return text.data(); }

    void SetType(int t) { type = (char)t; }
    void SetSubtype(int s) { subtype = (char)s; }
    void SetOpposingType(int t) { opp_type = (char)t; }
    void SetIFF(int t) { team = (char)t; }
    void SetStatus(int s) { status = (char)s; }
    void SetSource(ECombatEventSource s) { source = s; }
    void SetLocation(const FVector& p) { loc = p; }
    void SetSystem(Text sys) { system = sys; }
    void SetRegion(Text rgn) { region = rgn; }
    void SetFilename(Text f) { text_file = f; }
    void SetImageFile(Text f) { image_file = f; }
    void SetSceneFile(Text f) { scene_file = f; }
    void SetCount(int n) { count = (char)n; }
    void SetExecTime(int t) { time = t; }
    void SetStartBefore(int s) { start_before = s; }
    void SetStartAfter(int s) { start_after = s; }
    void SetMinRank(int n) { min_rank = (char)n; }
    void SetMaxRank(int n) { max_rank = (char)n; }
    void SetDelay(int d) { delay = d; }
    void SetProbability(int n) { probability = n; }
    void SetAssetType(int t) { asset_type = t; }
    void SetAssetId(int n) { asset_id = n; }
    void SetTargetType(int t) { target_type = t; }
    void SetTargetId(int n) { target_id = n; }
    void SetTargetIFF(int n) { target_iff = n; }
    void SetText(Text t) { text = t; }
    void SetTitle(const char* t);
    void SetSoundFile(const char* s);



private:
    int     id = 0;
    char    type = 0;
    char    subtype = 0;
    char    opp_type = 0;
    char    team = 0;
    char    status = PENDING;
    char    min_rank = 0;
    char    max_rank = 100;

    ECombatEventSource  source = ECombatEventSource::NONE;
    FVector loc;

    Text    title;   
    Text    system;
    Text    region;
    Text    text_file;
    Text    image_file;
    Text    scene_file;
    Text    sound_file;

    char    count = 0;
    int     start_before = TIME_NEVER;
    int     start_after = 0;
    int     delay = 0;
    int     probability = 0;
    int     rval = -1;
    int     time = 0;

    Text    text;

    int     asset_type = 0;
    int     asset_id = 0;
    List<Text> asset_kills;

    int     target_type = 0;
    int     target_id = 0;
    int     target_iff = 0;
    List<Text> target_kills;

    List<CombatActionReq> requirements;
};

// +--------------------------------------------------------------------+

class CombatActionReq
{
public:
    static const char* TYPENAME() { return "CombatActionReq"; }

    enum COMPARISON_OPERATOR
    {
        LT, LE, GT, GE, EQ,    // absolute
        RLT, RLE, RGT, RGE, REQ    // relative
    };

    CombatActionReq(int a, int s, bool n = false)
        : action(a)
        , stat(s)
        , notx(n)
        , c1(nullptr)
        , c2(nullptr)
        , comp(0)
        , score(0)
        , intel(0)
        , group_type(ECOMBATGROUP_TYPE::NONE) 
        , group_id(0)
    {
    }

    CombatActionReq(Combatant* a1, Combatant* a2, int comparison, int value)
        : action(0)
        , stat(0)
        , notx(false)
        , c1(a1)
        , c2(a2)
        , comp(comparison)
        , score(value)
        , intel(0)
        , group_type(ECOMBATGROUP_TYPE::NONE) 
        , group_id(0)
    {
    }

    CombatActionReq(
        Combatant* a1,
        ECOMBATGROUP_TYPE gtype,
        int gid,
        int comparison,
        int value,
        int intel_level = 0)
        : action(0)
        , stat(0)
        , notx(false)
        , c1(a1)
        , c2(nullptr)
        , comp(comparison)
        , score(value)
        , intel(intel_level)
        , group_type(gtype)   
        , group_id(gid)
    {
    }

    static int CompFromName(const char* sym);

    int        action = 0;
    int        stat = 0;
    bool       notx = false;

    Combatant* c1 = nullptr;
    Combatant* c2 = nullptr;

    int        comp = 0;
    int        score = 0;
    int        intel = 0;

    ECOMBATGROUP_TYPE group_type;
    int        group_id = 0;
};
