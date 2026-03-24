/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         OrbitalBody.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    OrbitalBody
    - Concrete orbital object (star, planet, moon)
    - Extends Orbital with lighting, rings, tilt, textures, and satellites
    - All implementation remains in StarSystem.cpp for now
*/

#pragma once

#include "Types.h"
#include "Text.h"
#include "List.h"

#include "Orbital.h"

// Forward declarations:
class StarSystem;
class SimLight;

// --------------------------------------------------------------------

class OrbitalBody : public Orbital
{
    friend class StarSystem;

public:
    static const char* TYPENAME() { return "OrbitalBody"; }

    OrbitalBody(
        StarSystem* sys,
        const char* n,
        OrbitalType t,
        double m,
        double r,
        double o,
        Orbital* prime = 0
    );

    virtual ~OrbitalBody();

    // operations:
    virtual void Update();

    // accessors:
    ListIter<OrbitalBody> Satellites() { return satellites; }

    double Tilt()     const { return tilt; }
    double RingMin()  const { return ring_min; }
    double RingMax()  const { return ring_max; }

    double LightIntensity() const { return light; }
    FColor LightColor()     const { return color; }
    bool   Luminous()       const { return luminous; }

protected:
    // texture / map identifiers:
    Text   map_name;
    Text   tex_name;
    Text   tex_high_res;
    Text   tex_ring;
    Text   tex_glow;
    Text   tex_glow_high_res;
    Text   tex_gloss;

    // physical / visual properties:
    double tscale;
    double light;
    double ring_min;
    double ring_max;
    double tilt;

    // lighting representations:
    SimLight* light_rep;
    SimLight* back_light;

    // colors:
    FColor  color;
    FColor  back;
    FColor  atmosphere;

    bool   luminous;

    // satellites (moons):
    List<OrbitalBody> satellites;
};
