/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         ShipLoad.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Ship loadout definition.

    - Defines weapon assignment per hardpoint station
    - Used by fighter-style ships with modular ordnance
*/

#pragma once

#include <cstring>

class ShipLoad
{
public:
    static const char* TYPENAME() { return "ShipLoad"; }

public:
    ShipLoad();

    // identity
    const char* GetName() const { return name; }
    void SetName(const char* n);

    // station access
    int  GetStation(int index) const;
    void SetStation(int index, int value);

    // mass
    double GetMass() const { return mass; }
    void   SetMass(double m) { mass = m; }

    int GetLoad(int Index) const
    {
        if (Index < 0 || Index >= 16)
            return 0;

        return load[Index];
    }
    int* GetLoad()
    {
        return load;
    }

    const int* GetLoad() const
    {
        return load;
    }


private:
    char   name[64];
    int    load[16];
    double mass;
};
