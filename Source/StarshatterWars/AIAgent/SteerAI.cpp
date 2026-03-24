/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         SteerAI.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Steering (low-level) Artificial Intelligence class
*/

#include "SteerAI.h"
#include "SeekerAI.h"
#include "FighterAI.h"
#include "StarshipAI.h"
#include "GroundAI.h"
#include "SimSystem.h"

#include "Game.h"
#include "Physical.h"

#include "CoreMinimal.h"              // UE_LOG, basic UE types
#include "Math/UnrealMathUtility.h"   // FMath
#include "Math/Vector.h"              // FVector

// +----------------------------------------------------------------------+

Steer
Steer::operator+(const Steer& s) const
{
    return Steer(yaw + s.yaw, pitch + s.pitch, roll + s.roll, (brake > s.brake) ? brake : s.brake);
}

Steer
Steer::operator-(const Steer& s) const
{
    return Steer(yaw - s.yaw, pitch - s.pitch, roll - s.roll, (brake < s.brake) ? brake : s.brake);
}

Steer
Steer::operator*(double f) const
{
    return Steer(yaw * f, pitch * f, roll * f, brake);
}

Steer
Steer::operator/(double f) const
{
    return Steer(yaw / f, pitch / f, roll / f, brake);
}

Steer&
Steer::operator+=(const Steer& s)
{
    yaw += s.yaw;
    pitch += s.pitch;
    roll += s.roll;

    if (s.brake > brake)
        brake = s.brake;

    if (s.stop)
        stop = 1;

    return *this;
}

Steer&
Steer::operator-=(const Steer& s)
{
    yaw -= s.yaw;
    pitch -= s.pitch;
    roll -= s.roll;

    if (s.brake < brake)
        brake = s.brake;

    if (s.stop)
        stop = 1;

    return *this;
}

double
Steer::Magnitude() const
{
    return sqrt(yaw * yaw + pitch * pitch);
}

// +--------------------------------------------------------------------+

SimDirector*
SteerAI::Create(SimObject* self, int type)
{
    switch (type) {
    case SEEKER:   return new SeekerAI(self);
    case STARSHIP: return new StarshipAI(self);
    case GROUND:   return new GroundAI(self);

    default:
    case FIGHTER:  return new FighterAI(self);
    }
}

// +----------------------------------------------------------------------+

SteerAI::SteerAI(SimObject* ship)
    : self(ship),
    target(0),
    subtarget(0),
    other(0),
    obj_w(FVector::ZeroVector),
    objective(FVector::ZeroVector),
    distance(0.0),
    magnitude(0),
    evade_time(0),
    seeking(0),
    seek_gain(20),
    seek_damp(0.5),
    ai_type(0)
{
    for (int i = 0; i < 3; i++)
        az[i] = el[i] = 0;
}

// +--------------------------------------------------------------------+

SteerAI::~SteerAI()
{
}

// +--------------------------------------------------------------------+

void
SteerAI::SetTarget(SimObject* targ, SimSystem* sub)
{
    if (target != targ) {
        target = targ;

        if (target)
            Observe(target);
    }

    subtarget = sub;
}

void
SteerAI::DropTarget(double dtime)
{
    SetTarget(0);
}

// +--------------------------------------------------------------------+

bool
SteerAI::Update(SimObject* obj)
{
    if (obj == target) {
        target = 0;
        subtarget = 0;
    }

    if (obj == other) {
        other = 0;
    }

    return SimObserver::Update(obj);
}

const char*
SteerAI::GetObserverName() const
{
    // UE-safe, thread-local static buffer (avoids sprintf_s dependency):
    static thread_local char NameBuf[64];

#if PLATFORM_WINDOWS
    _snprintf_s(NameBuf, sizeof(NameBuf), _TRUNCATE, "SteerAI(%s)", self ? self->Name() : "null");
#else
    snprintf(NameBuf, sizeof(NameBuf), "SteerAI(%s)", self ? self->Name() : "null");
#endif

    return NameBuf;
}

// +--------------------------------------------------------------------+

FVector
SteerAI::ClosingVelocity()
{
    if (self) {
        if (target)
            return self->Velocity() - target->Velocity();
        else
            return self->Velocity();
    }

    return FVector(1, 0, 0);
}

void
SteerAI::FindObjective()
{
    if (!self || !target)
        return;

    FVector Cv = ClosingVelocity();
    double  Cvl = Cv.Length();
    double  Time = 0;

    if (Cvl > 5) {
        // distance from self to target:
        distance = FVector(target->Location() - self->Location()).Length();

        // time to reach target:
        Time = distance / Cvl;

        // where the target will be when we reach it:
        FVector RunVec = target->Velocity();
        obj_w = target->Location() + (RunVec * Time);
    }
    else {
        obj_w = target->Location();
    }

    // subsystem offset:
    if (subtarget) {
        FVector Offset = target->Location() - subtarget->MountLocation();
        obj_w -= Offset;
    }

    distance = FVector(obj_w - self->Location()).Length();

    if (Cvl > 5)
        Time = distance / Cvl;

    // where we will be when the target gets there:
    FVector SelfDest = self->Location() + Cv * Time;
    FVector Err = obj_w - SelfDest;

    obj_w += Err;

    // transform into camera coords:
    objective = Transform(obj_w);
    objective.Normalize();

    distance = FVector(obj_w - self->Location()).Length();
}

FVector
SteerAI::Transform(const FVector& Pt)
{
    FVector ObjT = Pt - self->Location();
    FVector Result;

    if (self->FlightPathYawAngle() != 0 || self->FlightPathPitchAngle() != 0) {
        double Az = self->FlightPathYawAngle();
        double El = self->FlightPathPitchAngle();

        const double MAX_ANGLE = 15 * DEGREES;
        const double MIN_ANGLE = 3 * DEGREES;

        if (Az > MAX_ANGLE)            Az = MAX_ANGLE;
        else if (Az < -MAX_ANGLE)      Az = -MAX_ANGLE;
        else if (Az > MIN_ANGLE)       Az = MIN_ANGLE + (Az - MIN_ANGLE) / 2;
        else if (Az < -MIN_ANGLE)      Az = -MIN_ANGLE + (Az + MIN_ANGLE) / 2;

        if (El > MAX_ANGLE)            El = MAX_ANGLE;
        else if (El < -MAX_ANGLE)      El = -MAX_ANGLE;
        else if (El > MIN_ANGLE)       El = MIN_ANGLE + (El - MIN_ANGLE) / 2;
        else if (El < -MIN_ANGLE)      El = -MIN_ANGLE + (El + MIN_ANGLE) / 2;

        Camera Cam;
        Cam.Clone(self->Cam());
        Cam.Yaw(Az);
        Cam.Pitch(-El);

        Result =
            (ObjT * Cam.vrt()) +
            (ObjT * Cam.vup()) +
            (ObjT * Cam.vpn());
    }
    else {
        Camera& Cam = (Camera&)self->Cam(); // cast away const

        Result =
            (ObjT * Cam.vrt()) +
            (ObjT * Cam.vup()) +
            (ObjT * Cam.vpn());
    }

    return Result;
}

FVector
SteerAI::AimTransform(const FVector& Pt)
{
    Camera& Cam = (Camera&)self->Cam(); // cast away const
    FVector ObjT = Pt - self->Location();

    FVector Result =
        (ObjT * Cam.vrt()) +
        (ObjT * Cam.vup()) +
        (ObjT * Cam.vpn());

    return Result;
}

// +--------------------------------------------------------------------+

void
SteerAI::Navigator()
{
    accumulator.Clear();
    magnitude = 0;
}

int
SteerAI::Accumulate(const Steer& steer)
{
    int overflow = 0;

    double mag = steer.Magnitude();

    if (magnitude + mag > 1) {
        overflow = 1;
        double scale = (1 - magnitude) / mag;

        accumulator += steer * scale;
        magnitude = 1;

        if (seeking) {
            az[0] *= scale;
            el[0] *= scale;
            seeking = 0;
        }
    }
    else {
        accumulator += steer;
        magnitude += mag;
    }

    return overflow;
}

// +--------------------------------------------------------------------+

Steer
SteerAI::Seek(const FVector& Point)
{
    Steer s;

    // advance memory pipeline:
    az[2] = az[1]; az[1] = az[0];
    el[2] = el[1]; el[1] = el[0];

    // approach
    if (Point.Z > 0.0f) {
        az[0] = atan2(fabs(Point.X), Point.Z) * seek_gain;
        el[0] = atan2(fabs(Point.Y), Point.Z) * seek_gain;

        if (Point.X < 0) az[0] = -az[0];
        if (Point.Y > 0) el[0] = -el[0];

        s.yaw = az[0] - seek_damp * (az[1] + az[2] * 0.5);
        s.pitch = el[0] - seek_damp * (el[1] + el[2] * 0.5);
    }

    // reverse
    else {
        if (Point.X > 0) s.yaw = 1.0f;
        else             s.yaw = -1.0f;

        s.pitch = -Point.Y * 0.5f;
    }

    seeking = 1;

    return s;
}

// +--------------------------------------------------------------------+

Steer
SteerAI::Flee(const FVector& Pt)
{
    Steer s;

    FVector Point = Pt;
    Point.Normalize();

    // approach
    if (Point.Z > 0.0f) {
        if (Point.X > 0) s.yaw = -1.0f;
        else             s.yaw = 1.0f;
    }

    // flee
    else {
        s.yaw = -Point.X;
        s.pitch = Point.Y;
    }

    return s;
}

// +--------------------------------------------------------------------+

Steer
SteerAI::Avoid(const FVector& Point, float Radius)
{
    Steer s;

    if (Point.Z > 0) {
        double ax = Radius - fabs(Point.X);
        double ay = Radius - fabs(Point.Y);

        // go around?
        if (ax < ay) {
            s.yaw = atan2(ax, Point.Z) * seek_gain;
            if (Point.X > 0) s.yaw = -s.yaw;
        }

        // go over/under:
        else {
            s.pitch = atan2(ay, Point.Z) * seek_gain;
            if (Point.Y < 0) s.pitch = -s.pitch;
        }
    }

    return s;
}

// +--------------------------------------------------------------------+

Steer
SteerAI::Evade(const FVector& Point, const FVector& Vel)
{
    (void)Point;
    (void)Vel;

    Steer Evade;

    if (Game::GameTime() - evade_time > 1250) {
        evade_time = Game::GameTime();

        const int32 Direction = FMath::RandRange(0, 7);

        switch (Direction) {
        default:
        case 0:  Evade.yaw = 0;  Evade.pitch = -0.5; break;
        case 1:  Evade.yaw = 0;  Evade.pitch = -1.0; break;
        case 2:  Evade.yaw = 1;  Evade.pitch = -0.3; break;
        case 3:  Evade.yaw = 1;  Evade.pitch = -0.6; break;
        case 4:  Evade.yaw = 1;  Evade.pitch = -1.0; break;
        case 5:  Evade.yaw = -1; Evade.pitch = -0.3; break;
        case 6:  Evade.yaw = -1; Evade.pitch = -0.6; break;
        case 7:  Evade.yaw = -1; Evade.pitch = -1.0; break;
        }
    }

    return Evade;
}
