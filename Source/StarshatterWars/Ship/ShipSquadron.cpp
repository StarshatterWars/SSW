/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipSquadron.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Squadron inventory definition used by carriers
    and hangar systems.
*/

#include "ShipSquadron.h"
#include "ShipDesign.h"

#include <cstring>

// +--------------------------------------------------------------------+

ShipSquadron::ShipSquadron()
    : design(nullptr),
    count(4),
    avail(4)
{
    name[0] = 0;
    count = 4;
    avail = 4;
}

// +--------------------------------------------------------------------+

const char*
ShipSquadron::GetName() const
{
    return name;
}

ShipDesign*
ShipSquadron::GetDesign() const
{
    return design;
}

int
ShipSquadron::GetCount() const
{
    return count;
}

int
ShipSquadron::GetAvail() const
{
    return avail;
}

// +--------------------------------------------------------------------+

void
ShipSquadron::SetName(const char* n)
{
    if (!n)
    {
        name[0] = 0;
        return;
    }

    strncpy_s(name, n, 63);
    name[63] = 0;
}

void
ShipSquadron::SetDesign(ShipDesign* d)
{
    design = d;
}

void
ShipSquadron::SetCount(int c)
{
    count = c;
}

void
ShipSquadron::SetAvail(int a)
{
    avail = a;
}