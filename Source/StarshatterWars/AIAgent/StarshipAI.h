/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         FighterAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Starship (low-level) Artificial Intelligence class
*/

#pragma once

#include "Math/Vector.h"   // FVector

#include "Types.h"
#include "ShipAI.h"

// +--------------------------------------------------------------------+

class SimObject;
class System;

class StarshipAI : public ShipAI
{
public:
    StarshipAI(SimObject* s);
    virtual ~StarshipAI();

    // convert the goal point from world to local coords:
    virtual void      FindObjective() override;

protected:
    // accumulate behaviors:
    virtual void      Navigator() override;
    virtual Steer     SeekTarget() override;
    virtual Steer     AvoidCollision() override;

    // steering functions:
    virtual Steer     Seek(const FVector& Point);
    virtual Steer     Flee(const FVector& Point);
    virtual Steer     Avoid(const FVector& Point, float Radius);

    virtual FVector   Transform(const FVector& Pt);

    // fire on target if appropriate:
    virtual void      FireControl() override;
    virtual void      HelmControl() override;
    virtual void      ThrottleControl() override;

    SimSystem*          SelectSubtarget();
    bool                AssessTargetPointDefense();

    DWORD               sub_select_time = 0;
    DWORD               point_defense_time = 0;
    SimSystem*          subtarget = nullptr;
    bool                tgt_point_defense = false;
};

// +--------------------------------------------------------------------+
