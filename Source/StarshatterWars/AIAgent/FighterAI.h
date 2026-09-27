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
    Fighter (low-level) Artificial Intelligence class
*/

#pragma once

#include "Types.h"
#include "ShipAI.h"

#include "Math/Vector.h" // FVector

// +--------------------------------------------------------------------+

class Ship;
class Shot;
class InboundSlot;

// +--------------------------------------------------------------------+

class FighterAI : public ShipAI
{
public:
    FighterAI(SimObject* s);
    virtual ~FighterAI();

    virtual void      ExecFrame(double seconds) override;
    virtual int       Subframe() const override { return true; }

    // convert the goal point from world to local coords:
    virtual void      FindObjective() override;
    virtual void      FindObjectiveNavPoint();

protected:
    // behaviors:
    virtual Steer     AvoidTerrain();
    virtual Steer     SeekTarget() override;
    virtual Steer     EvadeThreat();
    virtual FVector   ClosingVelocity();

    // accumulate behaviors:
    virtual void      Navigator() override;

    // steering functions:
    virtual Steer     Seek(const FVector& Point);
    virtual Steer     SeekFormationSlot();

    // fire on target if appropriate:
    virtual void      FireControl() override;
    virtual void      HelmControl() override;
    virtual void      ThrottleControl() override;

    virtual double    CalcDefensePerimeter(Ship* starship);
    virtual void      ReturnToBase(Ship* controller);

    SimShot*            decoy_missile = nullptr;
    double              missile_time = 0.0;
    int                 terrain_warning = 0;
    int                 drop_state = 0;
    char                dir_info[32] = { 0 };
    double              brakes = 0.0;
    double              z_shift = 0.0;
    double              time_to_dock = 0.0;
    InboundSlot*        inbound = nullptr;
    int                 rtb_code = 0;
    bool                evading = false;

    // UE-compatible time type (legacy code used DWORD):
    uint32            jink_time = 0;

    FVector           jink = FVector::ZeroVector;
    bool              over_threshold = false;
    bool              form_up = false;
    bool              go_manual = false;
};

// +--------------------------------------------------------------------+
