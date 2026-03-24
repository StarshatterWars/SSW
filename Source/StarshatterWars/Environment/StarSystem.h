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
    - Implementation remains in StarSystem.cpp
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

// Optional: bodies/regions are now split headers (thin, implementation stays in StarSystem.cpp)
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

    // operations:
    virtual void   Load();
    virtual void   Create();
    virtual void   Destroy();

    virtual void   Activate(SimScene& scene);
    virtual void   Deactivate();

    virtual void   ExecFrame();

    // accessors:
    const char* Name()         const { return name; }
    const char* Govt()         const { return govt; }
    const char* Description()  const { return description; }
    int            Affiliation()  const { return affiliation; }
    int            Sequence()     const { return seq; }
    FVector        Location()     const { return loc; }
    int            NumStars()     const { return sky_stars; }
    int            NumDust()      const { return sky_dust; }
    FColor         Ambient()      const;

    List<OrbitalBody>& Bodies() { return bodies; }
    List<OrbitalRegion>& Regions() { return regions; }
    List<OrbitalRegion>& AllRegions() { return all_regions; }
    OrbitalRegion* ActiveRegion() { return active_region; }

    Orbital* FindOrbital(const char* in_name);
    OrbitalRegion* FindRegion(const char* in_name);

    void                  SetActiveRegion(OrbitalRegion* rgn);

    static void           SetBaseTime(double t, bool absolute = false);
    static double         GetBaseTime();
    static double         Stardate() { return stardate; }
    static void           CalcStardate();

    double                Radius() const { return radius; }

    void                  SetSunlight(FColor color, double brightness = 1);
    void                  SetBacklight(FColor color, double brightness = 1);
    void                  RestoreTrueSunColor();

    bool                  HasLinkTo(StarSystem* s) const;

    const Text& GetDataPath() const { return datapath; }

protected:
    // parsing:
    void                  ParseStar(TermStruct* val);
    void                  ParsePlanet(TermStruct* val);
    void                  ParseMoon(TermStruct* val);
    void                  ParseRegion(TermStruct* val);
    void                  ParseTerrain(TermStruct* val);
    void                  ParseLayer(TerrainRegion* rgn, TermStruct* val);

    // creation helpers:
    void                  CreateBody(OrbitalBody& body);
    FVector               TerrainTransform(const FVector& in_loc);

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
};

// +--------------------------------------------------------------------+

class Star
{
public:
    static const char* TYPENAME() { return "Star"; }

    Star(const char* n, const FVector& l, int s) : name(n), loc(l), seq(s) {}
    virtual ~Star() {}

    enum SPECTRAL_CLASS {
        BLACK_HOLE, WHITE_DWARF, RED_GIANT,
        O, B, A, F, G, K, M
    };

    int operator == (const Star& s) const { return name == s.name; }

    const char* Name()      const { return name; }
    const FVector& Location()  const { return loc; }
    int                   Sequence()  const { return seq; }

    FColor                GetColor() const;
    int                   GetSize()  const;

    static FColor         GetColor(int spectral_class);
    static int            GetSize(int spectral_class);

protected:
    Text                  name;
    FVector               loc;
    int                   seq;
};
