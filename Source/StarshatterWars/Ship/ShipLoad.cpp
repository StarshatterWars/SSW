/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         ShipLoad.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Ship loadout definition.

    - Defines weapon assignment per hardpoint station
    - Used by fighter-style ships with modular ordnance
*/

#include "ShipLoad.h"

ShipLoad::ShipLoad()
{
    name[0] = 0;

    for (int i = 0; i < 16; i++)
    {
        load[i] = -1;
    }

    mass = 0.0;
}

void ShipLoad::SetName(const char* n)
{
    if (!n)
    {
        name[0] = 0;
        return;
    }

    strncpy(name, n, 63);
    name[63] = 0;
}

int ShipLoad::GetStation(int index) const
{
    if (index < 0 || index >= 16)
    {
        return -1;
    }

    return load[index];
}

void ShipLoad::SetStation(int index, int value)
{
    if (index < 0 || index >= 16)
    {
        return;
    }

    load[index] = value;
}

