/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         SeekerAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Seeker Missile (low-level) Artificial Intelligence class
*/

#pragma once

#include "Types.h"
#include "SteerAI.h"
#include "SimObject.h"

// +--------------------------------------------------------------------+

class Ship;
class SimShot;
class System;

class SeekerAI : public SteerAI
{
public:
    SeekerAI(SimObject* s);
    virtual ~SeekerAI();

    virtual int       Type() const override { return 1001; }
    virtual int       Subframe() const override { return true; }

    virtual void      ExecFrame(double seconds) override;
    virtual void      FindObjective() override;
    virtual void      SetTarget(SimObject* targ, SimSystem* sub = 0) override;
    virtual bool      Overshot();

    virtual void      SetPursuit(int p) { pursuit = p; }
    virtual int       GetPursuit() const { return pursuit; }

    virtual void      SetDelay(double d) { delay = d; }
    virtual double    GetDelay() const { return delay; }

    virtual bool         Update(SimObject* obj) override;
    virtual const char* GetObserverName() const override;

protected:
    // behaviors:
    virtual Steer     AvoidCollision();
    virtual Steer     SeekTarget();

    // accumulate behaviors:
    virtual void      Navigator() override;

    virtual void      CheckDecoys(double distance);

    Ship* orig_target = nullptr;
    SimShot* shot = nullptr;
    int               pursuit = 0;    // type of pursuit curve
    // 1: pure pursuit
    // 2: lead pursuit

    double            delay = 0.0;    // don't start seeking until then
    bool              overshot = false;
};

// +--------------------------------------------------------------------+
