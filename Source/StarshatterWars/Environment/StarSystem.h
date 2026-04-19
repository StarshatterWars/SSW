/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         StarSystem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    StarSystem
    - Defines a stellar system and its orbiting objects (bodies, regions, terrain)
    - Parses system definition data and provides runtime access to orbitals
    - Also supports runtime hydration from Galaxy table data
*/

#pragma once

#include "Types.h"
#include "Text.h"
#include "Term.h"
#include "List.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Math/Color.h"

// Core orbital base:
#include "Orbital.h"

// Split headers:
#include "OrbitalBody.h"
#include "OrbitalRegion.h"

// +--------------------------------------------------------------------+

class Bitmap;
class TerrainRegion;

class Graphic;
class SimLight;
class SimScene;
class Solid;

// +--------------------------------------------------------------------+

class StarSystem
{
public:
    static const char* TYPENAME() { return "StarSystem"; }

    StarSystem(const char* in_name, FVector in_loc, int iff = 0, int s = 4);
    virtual ~StarSystem();

    int operator == (const StarSystem& s) const { return name == s.name; }

    // -----------------------------------------------------------------
    // operations:
    // -----------------------------------------------------------------
    virtual void   Load();
    virtual void   Create();
    virtual void   Destroy();

    virtual void   Activate(SimScene& scene);
    virtual void   Deactivate();

    virtual void   ExecFrame();

    // -----------------------------------------------------------------
    // accessors:
    // -----------------------------------------------------------------
    const char* GetName()           const { return name; }
    const char* GetGovt()           const { return govt; }
    const char* GetDescription()    const { return description; }
    int             GetAffiliation()    const { return affiliation; }
    int             GetSequence()       const { return seq; }
    FVector         GetLocation()       const { return loc; }
    int             GetNumStars()       const { return sky_stars; }
    int             GetNumDust()        const { return sky_dust; }
    FColor          GetAmbient()        const;

    List<OrbitalBody>& GetBodies() { return bodies; }
    List<OrbitalRegion>& GetRegions() { return regions; }
    List<OrbitalRegion>& GetAllRegions() { return all_regions; }
    OrbitalRegion* ActiveRegion() { return active_region; }

    Orbital* FindOrbital(const char* in_name);
    OrbitalRegion* FindRegion(const char* in_name);

    static void   SetSimulationTime(double t);
    static double GetSimulationTime();

    static void   SetBaseTime(double t, bool absolute = false);
    static double GetBaseTime();
    static double GetStardate() { return stardate; }
    static void   CalcStardate();

    double        GetRadius() const { return radius; }

    bool          HasLinkTo(StarSystem* s) const;

    const Text& GetDataPath() const { return datapath; }

    void RecalculateRadius();

    void AddRootRegion(OrbitalRegion* Region);

    // -----------------------------------------------------------------
    // runtime hydration helpers:
    // -----------------------------------------------------------------
    Orbital* GetCenter() const;

    void          AddBody(OrbitalBody* Body);
    void          AddRegion(OrbitalRegion* Region);

    void          SetName(const char* InName) { if (InName) name = InName; }
    void          SetGovt(const char* InGovt) { if (InGovt) govt = InGovt; }
    void          SetDescription(const char* InDesc) { if (InDesc) description = InDesc; }
    void          SetDataPath(const char* InPath) { if (InPath) datapath = InPath; }

    void          SetAffiliation(int InAffiliation) { affiliation = InAffiliation; }
    void          SetSequence(int InSeq) { seq = InSeq; }
    void          SetLocation(const FVector& InLoc) { loc = InLoc; }

    void          SetSkyCounts(int InStars, int InDust)
    {
        sky_stars = InStars;
        sky_dust = InDust;
    }

    void          SetSkyTextures(
        const char* InPolyStars,
        const char* InNebula,
        const char* InHaze)
    {
        sky_poly_stars = InPolyStars ? InPolyStars : "";
        sky_nebula = InNebula ? InNebula : "";
        sky_haze = InHaze ? InHaze : "";
    }

    void          SetAmbientColor(const FColor& InAmbient)
    {
        ambient = InAmbient;
    }

    void          SetSunlight(FColor color, double brightness = 1);
    void          SetBacklight(FColor color, double brightness = 1);
    void          RestoreTrueSunColor();

    void          SetActiveRegion(OrbitalRegion* rgn);

    // -----------------------------------------------------------------
    // runtime hydration from subsystem data:
    // -----------------------------------------------------------------
    void HydrateFromEnvironment(
        const FS_Galaxy& GalaxyRow,
        const FS_StarSystem* OptionalSystemMeta = nullptr);

    void ResetHydratedContents();

protected:
    // -----------------------------------------------------------------
    // parsing:
    // -----------------------------------------------------------------
    void          ParseStar(TermStruct* val);
    void          ParsePlanet(TermStruct* val);
    void          ParseMoon(TermStruct* val);
    void          ParseRegion(TermStruct* val);
    void          ParseTerrain(TermStruct* val);
    void          ParseLayer(TerrainRegion* rgn, TermStruct* val);

    // -----------------------------------------------------------------
    // creation helpers:
    // -----------------------------------------------------------------
    void          CreateBody(OrbitalBody& body);
    FVector       TerrainTransform(const FVector& in_loc);

protected:
    char                  filename[64];

    Text                  name;
    Text                  govt;
    Text                  description;
    Text                  datapath;

    int                   affiliation;
    int                   seq;

    FVector               loc;

    static double         stardate;
    double                radius;

    bool                  instantiated;

    // sky:
    int                   sky_stars;
    int                   sky_dust;

    Text                  sky_poly_stars;
    Text                  sky_nebula;
    Text                  sky_haze;

    double                sky_uscale;
    double                sky_vscale;

    // lighting:
    FColor                ambient;
    FColor                sun_color;
    double                sun_brightness;
    double                sun_scale;

    List<SimLight>        sun_lights;
    List<SimLight>        back_lights;

    // visuals:
    Graphic* point_stars;
    Solid* poly_stars;
    Solid* nebula;
    Solid* haze;

    // orbitals:
    List<OrbitalBody>     bodies;
    List<OrbitalRegion>   regions;
    List<OrbitalRegion>   all_regions;

    Orbital* center;
    OrbitalRegion* active_region;

    // terrain view basis:
    FVector               tvpn;
    FVector               tvup;
    FVector               tvrt;

private:
    int32 ToLegacyStarClass(ESPECTRAL_CLASS InClass) const;

    OrbitalBody* HydrateStar(const FStarSystem& Row);
    OrbitalBody* HydratePlanet(OrbitalBody* ParentStar, const FPlanet& Row);
    OrbitalBody* HydrateMoon(OrbitalBody* ParentPlanet, const FMoon& Row);
    OrbitalRegion* HydrateRegion(Orbital* Parent, const FRegion& Row);
};

// +--------------------------------------------------------------------+

class Star
{
public:
    static const char* TYPENAME() { return "Star"; }

    Star(const char* n, const FVector& l, int s) : name(n), loc(l), seq(s) {}
    virtual ~Star() {}

    enum SPECTRAL_CLASS
    {
        BLACK_HOLE, WHITE_DWARF, RED_GIANT,
        O, B, A, F, G, K, M
    };

    int operator == (const Star& s) const { return name == s.name; }

    const char* GetName()     const { return name; }
    const FVector& Location()    const { return loc; }
    int            Sequence()    const { return seq; }

    FColor         GetColor() const;
    int            GetSize()  const;

    static FColor  GetColor(int spectral_class);
    static int     GetSize(int spectral_class);

protected:
    Text           name;
    FVector        loc;
    int            seq;
};