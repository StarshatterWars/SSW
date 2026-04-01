/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionLoad.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionLoad implementation

    Provides storage and access for runtime weapon loadouts.
*/

#include "MissionLoad.h"
#include "Mission.h"
#include "MissionElement.h"
#include "ParseUtil.h"
#include "FormatUtil.h"

// +--------------------------------------------------------------------+

MissionLoad::MissionLoad(int s, const char* n)
    : ship(s)
{
    Clear();

    if (n)
    {
        name = n;
    }
}

MissionLoad::~MissionLoad()
{
}

// +--------------------------------------------------------------------+

int MissionLoad::GetShip() const
{
    return ship;
}

void MissionLoad::SetShip(int s)
{
    ship = s;
}

// +--------------------------------------------------------------------+

Text MissionLoad::GetName() const
{
    return name;
}

void MissionLoad::SetName(Text n)
{
    name = n;
}

// +--------------------------------------------------------------------+

int* MissionLoad::GetStations()
{
    return load;
}

const int* MissionLoad::GetStations() const
{
    return load;
}

// +--------------------------------------------------------------------+

int MissionLoad::GetStation(int index) const
{
    if (index >= 0 && index < MAX_STATIONS)
    {
        return load[index];
    }

    return -1; // no weapon
}

void MissionLoad::SetStation(int index, int selection)
{
    if (index >= 0 && index < MAX_STATIONS)
    {
        load[index] = selection;
    }
}

// +--------------------------------------------------------------------+

int MissionLoad::GetNumStations() const
{
    return MAX_STATIONS;
}

void MissionLoad::Clear()
{
    for (int i = 0; i < MAX_STATIONS; ++i)
    {
        load[i] = -1; // no weapon mounted
    }
}

