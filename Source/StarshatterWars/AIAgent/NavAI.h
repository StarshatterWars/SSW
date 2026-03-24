/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         NavAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Automatic Navigator
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "SimSystem.h"
#include "ShipAI.h"
#include "Text.h"

#include "Math/Vector.h" // FVector

// +--------------------------------------------------------------------+

class Farcaster;
class Ship;

// +--------------------------------------------------------------------+

class NavAI : public ShipAI
{
public:
    NavAI(Ship* s);
    virtual ~NavAI();

    enum { DIR_TYPE = 2000 };
    virtual int       Type() const override { return DIR_TYPE; }

    virtual void      ExecFrame(double seconds) override;
    virtual int       Subframe() const override { return true; }
    void              Disengage();
    bool              Complete() const { return complete; }

protected:
    // behaviors:
    virtual Steer     SeekTarget() override;

    // steering functions:
    virtual FVector   Transform(const FVector& Pt);
    virtual Steer     Seek(const FVector& Point);
    virtual Steer     Flee(const FVector& Point);
    virtual Steer     Avoid(const FVector& Point, float Radius);
    virtual Steer     AvoidTerrain();

    // accumulate behaviors:
    virtual void      Navigator() override;
    virtual void      FindObjective() override;

    virtual void      HelmControl() override;
    virtual void      ThrottleControl() override;

    bool              complete = false;
    int               drop_state = 0;
    int               quantum_state = 0;
    int               terrain_warning = 0;
    double            brakes = 0.0;
    Farcaster*        farcaster = nullptr;
};

// +--------------------------------------------------------------------+
