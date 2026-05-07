/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipSquadron.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Squadron inventory definition used by carriers
    and hangar systems.
*/

#pragma once

#include "Types.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class ShipDesign;

// +--------------------------------------------------------------------+

class ShipSquadron
{
public:
    static const char* TYPENAME() { return "ShipSquadron"; }

    ShipSquadron();

    // accessors:
    const char* GetName()   const;
    ShipDesign* GetDesign() const;
    int         GetCount()  const;
    int         GetAvail()  const;

    void SetName(const char* n);
    void SetDesign(ShipDesign* d);
    void SetCount(int c);
    void SetAvail(int a);

protected:
    char         name[64];
    ShipDesign* design;
    int          count;
    int          avail;
}; 
