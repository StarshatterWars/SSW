/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         SteerAI.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Steering (low-level) Artificial Intelligence class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "SimDirector.h"
#include "Geometry.h"

#include "Math/Vector.h" // FVector

// +--------------------------------------------------------------------+

class SimSystem;

// +--------------------------------------------------------------------+

struct Steer
{
    Steer() : yaw(0), pitch(0), roll(0), brake(0), stop(0) {}
    Steer(double y, double p, double r, double b = 0) : yaw(y), pitch(p), roll(r), brake(b), stop(0) {}
    Steer(const Steer& s) : yaw(s.yaw), pitch(s.pitch), roll(s.roll), brake(s.brake), stop(s.stop) {}

    Steer& operator=(const Steer& s)
    {
        yaw = s.yaw; pitch = s.pitch; roll = s.roll; brake = s.brake; stop = s.stop;
        return *this;
    }

    Steer  operator+(const Steer& s) const;
    Steer  operator-(const Steer& s) const;
    Steer  operator*(double f)       const;
    Steer  operator/(double f)       const;

    Steer& operator+=(const Steer& s);
    Steer& operator-=(const Steer& s);

    double Magnitude() const;

    void Clear() { yaw = 0; pitch = 0; roll = 0; brake = 0; stop = 0; }

    double yaw, pitch, roll;
    double brake;
    int    stop;
};

// +--------------------------------------------------------------------+

class SteerAI : public SimDirector, public SimObserver
{
public:
    enum Type { SEEKER = 1000, FIGHTER, STARSHIP, GROUND };

    SteerAI(SimObject* self);
    virtual ~SteerAI();

    static SimDirector* Create(SimObject*, int type);

    virtual void       SetTarget(SimObject* targ, SimSystem* sub = 0);
    virtual SimObject* GetTarget() const { return target; }
    virtual SimSystem* GetSubTarget() const { return subtarget; }
    virtual void       DropTarget(double drop_time = 1.5);
    virtual int        Type() const { return ai_type; }

    virtual bool       Update(SimObject* obj);
    virtual const char* GetObserverName() const;

    // debug:
    virtual FVector     GetObjective() const { return obj_w; }
    virtual SimObject*  GetOther() const { return other; }

protected:
    // accumulate behaviors:
    virtual void      Navigator();
    virtual int       Accumulate(const Steer& steer);

    // steering functions:
    virtual Steer     Seek(const FVector& Point);
    virtual Steer     Flee(const FVector& Point);
    virtual Steer     Avoid(const FVector& Point, float Radius);
    virtual Steer     Evade(const FVector& Point, const FVector& Vel);

    // compute the goal point based on target stats:
    virtual void      FindObjective();
    virtual FVector   ClosingVelocity();

    virtual FVector   Transform(const FVector& Pt);
    virtual FVector   AimTransform(const FVector& Pt);

    int               seeking = 0;

    SimObject* self = nullptr;
    SimObject* target = nullptr;
    SimSystem* subtarget = nullptr;
    SimObject* other = nullptr;

    FVector           obj_w = FVector::ZeroVector;
    FVector           objective = FVector::ZeroVector;

    double            distance = 0;
    double            az[3] = { 0, 0, 0 };
    double            el[3] = { 0, 0, 0 };

    Steer             accumulator;
    double            magnitude = 0;

    // UE-compatible time type (legacy code used DWORD):
    uint32            evade_time = 0;

    double            seek_gain = 0;
    double            seek_damp = 0;

    int               ai_type = 0;
};

// +--------------------------------------------------------------------+
