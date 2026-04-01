/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionLoad.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionLoad

    Runtime weapon loadout container.

    This class represents the mutable, per-mission loadout
    assigned to a specific ship instance.

    ARCHITECTURE ROLE
    =================
    FShipDesign   -> static hardpoints definition
    FShipLoadout  -> preset templates
    MissionLoad   -> runtime mutable state (THIS CLASS)

    Each station index maps directly to a hardpoint index.
    The load array stores weapon selection IDs per station.

    NOTES
    =====
    - Fixed size of 16 stations (legacy constraint)
    - -1 indicates no weapon mounted
    - This class is NOT Unreal-facing
    - Convert to FMissionRuntimeLoadout for UI usage
*/

#pragma once

#include "Text.h"

class MissionLoad
{
public:
    static const char* TYPENAME() { return "MissionLoad"; }

public:
    MissionLoad(int ship = -1, const char* name = 0);
    ~MissionLoad();

    int  GetShip() const;
    void SetShip(int ship);

    Text GetName() const;
    void SetName(Text name);

    int* GetStations();
    const int* GetStations() const;

    int  GetStation(int index) const;
    void SetStation(int index, int selection);

    int  GetNumStations() const;
    void Clear();

private:
    static const int MAX_STATIONS = 16;

    int  ship;
    Text name;
    int  load[MAX_STATIONS];
};