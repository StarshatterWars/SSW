/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         StarshipTacticalAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Starship-specific mid-level (tactical) AI
*/

#pragma once

#include "Types.h"
#include "TacticalAI.h"

// +--------------------------------------------------------------------+

class Ship;
class ShipAI;

class StarshipTacticalAI : public TacticalAI
{
public:
    StarshipTacticalAI(ShipAI* ai);
    virtual ~StarshipTacticalAI();

    virtual void      ExecFrame(double seconds) override;

protected:
    virtual void      FindThreat() override;
    virtual void      FindSupport() override;

    virtual void      CheckBugOut(Ship* c_ship, double range);

    // NOTE: Use engine-agnostic fixed width type in UE builds instead of Win32 DWORD.
    // This is a plain C++ class, so keep it lightweight and portable.
    uint32            THREAT_REACTION_TIME = 0;

    int               ai_level = 0;
    double            drop_time = 0.0;
    double            initial_integrity = 0.0;
    bool              bugout = false;
};

// +--------------------------------------------------------------------+
