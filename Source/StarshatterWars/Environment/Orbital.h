/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         Orbital.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Orbital
    - Base class for orbital objects in a StarSystem
      (stars, planets, moons, regions, terrain)
    - Owns fundamental orbital parameters and predicted location
    - Holds child OrbitalRegion list (legacy behavior)
    - Pure C++ (non-UObject) simulation type
*/

#pragma once

#include "Types.h"
#include "Text.h"
#include "List.h"

// Minimal Unreal includes (per port rules):
#include "Math/Vector.h"   // FVector

// Forward declarations:
class StarSystem;
class Graphic;
class Bitmap;
class OrbitalRegion;

class Orbital
{
    friend class StarSystem;

public:
    static const char* TYPENAME() { return "Orbital"; }

    enum OrbitalType
    {
        NOTHING,
        STAR,
        PLANET,
        MOON,
        REGION,
        TERRAIN
    };

    Orbital(
        StarSystem* sys,
        const char* n,
        OrbitalType t,
        double m,
        double r,
        double o,
        Orbital* p = 0
    );

    virtual ~Orbital();

    int operator == (const Orbital& o) const
    {
        return type == o.type && name == o.name && system == o.system;
    }

    int operator < (const Orbital& o) const
    {
        return loc.Size() < o.loc.Size();
    }

    int operator <= (const Orbital& o) const
    {
        return loc.Size() <= o.loc.Size();
    }

    // operations:
    virtual void Update();
    FVector PredictLocation(double delta_t);

    // accessors:
    const char* Name() const { return name; }
    OrbitalType Type() const { return type; }
    int         SubType() const { return subtype; }

    const char* Description() const { return description; }
    double      Mass() const { return mass; }
    double      Radius() const { return radius; }
    double      Rotation() const { return rotation; }
    double      RotationPhase() const { return theta; }
    double      Orbit() const { return orbit; }
    bool        Retrograde() const { return retro; }
    double      Phase() const { return phase; }
    double      Period() const { return period; }
    FVector     Location() const { return loc; }

    Graphic* Rep() const { return rep; }

    Bitmap* GetMapIcon() const { return map_icon; }
    void    SetMapIcon(Bitmap* img) { map_icon = img; }

    StarSystem* System() const { return system; }
    Orbital* Primary() const { return primary; }

    // Legacy region iterator:
    ListIter<OrbitalRegion> Regions() { return regions; }

protected:
    // Identity and classification:
    Text        name;
    OrbitalType type;
    int         subtype;

    // Description and physical parameters:
    Text        description;
    double      mass;
    double      radius;
    double      rotation;
    double      theta;

    // Orbit parameters:
    double      orbit;
    double      phase;
    double      period;
    double      velocity;

    // State:
    FVector     loc;
    bool        retro;

    // Visuals:
    Graphic* rep;
    Bitmap* map_icon;

    // Hierarchy:
    StarSystem* system;
    Orbital* primary;

    // Child regions:
    List<OrbitalRegion> regions;
};
