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

public:

    const char* GetName() const { return name.data(); }
    OrbitalType GetType() const { return type; }

    double GetMass() const { return mass; }
    double GetRadius() const { return radius; }
    double GetOrbit() const { return orbit; }

    const char* GetMapName() const { return map_name.data(); }
    const char* GetTexture() const { return tex_name.data(); }
    const char* GetRingTexture() const { return tex_ring.data(); }

    double GetRotation() const { return rotation; }
    double GetTilt() const { return tilt; }

    double GetRingMin() const { return ring_min; }
    double GetRingMax() const { return ring_max; }

    FColor GetColor() const { return color; }
    FColor GetAtmosphere() const { return atmosphere; }

    int GetSubtype() const { return subtype; }

    
    

public:
    void SetMapName(const char* In) { map_name = In; }
    void SetTexture(const char* In) { tex_name = In; }
    void SetRingTexture(const char* In) { tex_ring = In; }
    void SetGlossTexture(const char* In) { tex_gloss = In; }

    void SetRotation(double In) { rotation = In; }
    void SetTilt(double In) { tilt = In; }

    void SetRingRange(double Min, double Max)
    {
        ring_min = Min;
        ring_max = Max;
    }

    void SetLighting(double Intensity, const FColor& InColor)
    {
        light = Intensity;
        color = InColor;
    }

    void SetBackColor(const FColor& InColor)
    {
        back = InColor;
    }

    void SetAtmosphere(const FColor& InColor)
    {
        atmosphere = InColor;
    }

    void SetSubtype(int InSubtype)
    {
        subtype = InSubtype;
    }

    void AddSatellite(OrbitalBody* Body)
    {
        if (Body)
        {
            satellites.append(Body);
        }
    }

    void SetRetro(bool bIn) { retro = bIn; }
    void SetTimeScale(double In) { tscale = In; }

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

    int subtype;
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
