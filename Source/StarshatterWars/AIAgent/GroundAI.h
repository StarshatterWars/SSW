/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         GroundAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Ground Unit (low-level) Artificial Intelligence class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "SimDirector.h"
#include "Geometry.h"

// +--------------------------------------------------------------------+

class Ship;
class SimSystem;
class CarrierAI;

// +--------------------------------------------------------------------+

class GroundAI : public SimDirector, public SimObserver
{
public:
    GroundAI(SimObject* self);
    virtual ~GroundAI();

    virtual void        ExecFrame(double seconds);
    virtual void        SetTarget(SimObject* targ, SimSystem* sub = 0);
    virtual SimObject*  GetTarget() const { return target; }
    virtual SimSystem*     GetSubTarget() const { return subtarget; }
    virtual int         Type() const;

    virtual bool        Update(SimObject* obj) override;
    virtual const char* GetObserverName() const override;

protected:
    virtual void      SelectTarget();

    Ship* ship = nullptr;
    SimObject* target = nullptr;
    SimSystem* subtarget = nullptr;
    double            exec_time = 0.0;
    CarrierAI* carrier_ai = nullptr;
};

// +--------------------------------------------------------------------+
