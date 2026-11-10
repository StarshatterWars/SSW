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
class OrbitalBody;

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
    virtual FVector PredictLocation(double delta_t);

    // accessors:
    const char* GetName()       const { return name; }
    OrbitalType GetType()       const { return type; }
    int         GetSubType()    const { return subtype; }

    const char* Description() const { return description; }
    double      Mass() const { return mass; }
    double      Radius() const { return radius; }
    double      Rotation() const { return rotation; }
    double      RotationPhase() const { return theta; }
    double      Orbit() const { return orbit; }
    double      Apoapsis() const { return orbit * (1.0 + eccentricity); }
    void SetOrbitShape(double AuthoredEccentricity, double InclinationDegrees);
    void BoundOrbit(ListIter<OrbitalBody> Siblings);
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

    void AddRegion(OrbitalRegion* Region)
    {
        if (Region)
        {
            regions.append(Region);
        }
    }

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

    double authored_eccentricity = 0.0;
    double eccentricity = 0.0;
    double orbit_inclination = 0.0;
    FVector OrbitOffset(double MeanAnomaly) const;

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
