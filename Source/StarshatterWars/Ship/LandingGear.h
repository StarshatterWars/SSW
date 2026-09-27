/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         LandingGear.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Fighter undercarriage (landing gear) system class

    Original Author and Studio:
    John DiCamillo / Destroyer Studios
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "Solid.h"
#include "SimModel.h"

// Minimal Unreal include for FVector (Point/Vec3 conversion target):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Ship;
class Physical;
class Solid;

// +--------------------------------------------------------------------+

class LandingGear : public SimSystem
{
public:
    enum CONSTANTS { MAX_GEAR = 4 };
    enum GEAR_STATE { GEAR_UP, GEAR_LOWER, GEAR_DOWN, GEAR_RAISE };

    LandingGear();
    LandingGear(const LandingGear& rhs);
    virtual ~LandingGear();

    virtual int       AddGear(SimModel* m, const FVector& s, const FVector& e);
    virtual void      ExecFrame(double seconds);
    virtual void      Orient(const Physical* rep);

    GEAR_STATE        GetState()        const { return state; }
    void              SetState(GEAR_STATE s);
    int               NumGear()         const { return ngear; }
    Solid* GetGear(int i);
    FVector           GetGearStop(int i);
    double            GetTouchDown();
    double            GetClearance()    const { return clearance; }

    static void       Initialize();
    static void       Close();

protected:
    GEAR_STATE        state;
    double            transit;
    double            clearance;

    int               ngear;
    SimModel*         models[MAX_GEAR];
    Solid*            gear[MAX_GEAR];
    FVector           start[MAX_GEAR];
    FVector           end[MAX_GEAR];
};
