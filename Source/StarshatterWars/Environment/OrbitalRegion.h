/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         OrbitalRegion.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    OrbitalRegion
    - Represents a non-body orbital space region
    - Used for asteroid belts, debris fields, navigation grids
    - Owned and managed by StarSystem
*/

#pragma once

#include "Orbital.h"
#include "List.h"
#include "Text.h"

// Forward declarations:
class StarSystem;

// --------------------------------------------------------------------
// OrbitalRegion
// --------------------------------------------------------------------

class OrbitalRegion : public Orbital
{
    friend class StarSystem;

public:
    static const char* TYPENAME() { return "OrbitalRegion"; }

    OrbitalRegion(
        StarSystem* sys,
        const char* n,
        double      mass,
        double      radius,
        double      orbit,
        Orbital* primary = 0
    );

    virtual ~OrbitalRegion();

    // Accessors:
    double      GridSpace()   const { return grid; }
    double      Inclination() const { return inclination; }
    int         Asteroids()   const { return asteroids; }

    List<Text>& Links() { return links; }
    const List<Text>& Links() const { return links; }

protected:
    // Region parameters:
    double      grid;          // grid spacing for nav / encounter logic
    double      inclination;   // orbital inclination (radians or degrees, legacy-defined)
    int         asteroids;     // asteroid density / count hint

    // Linked regions or zones:
    List<Text>  links;
};

