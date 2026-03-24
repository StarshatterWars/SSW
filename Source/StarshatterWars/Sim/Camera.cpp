/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    Original Author and Studio:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         Camera.cpp
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    Camera Class - Position and Point of View
*/

#include "Camera.h"

#include "Math/Vector.h"                // FVector
#include "Math/UnrealMathUtility.h"     // FMath, PI

// +--------------------------------------------------------------------+

Camera::Camera(double x, double y, double z)
    : pos((float)x, (float)y, (float)z)
{
}

Camera::~Camera()
{
}

// +--------------------------------------------------------------------+

void
Camera::MoveTo(double x, double y, double z)
{
    pos.X = (float)x;
    pos.Y = (float)y;
    pos.Z = (float)z;
}

void
Camera::MoveTo(const FVector& p)
{
    pos = p;
}

// +--------------------------------------------------------------------+

void
Camera::MoveBy(double dx, double dy, double dz)
{
    pos.X += (float)dx;
    pos.Y += (float)dy;
    pos.Z += (float)dz;
}

void
Camera::MoveBy(const FVector& p)
{
    pos += p;
}

// +--------------------------------------------------------------------+

void
Camera::Clone(const Camera& cam)
{
    pos = cam.pos;
    orientation = cam.orientation;
}

// +--------------------------------------------------------------------+

void
Camera::LookAt(const FVector& target, const FVector& eye, const FVector& up)
{
    FVector zaxis = target - eye;
    zaxis.Normalize();

    FVector xaxis = FVector::CrossProduct(up, zaxis);
    xaxis.Normalize();

    FVector yaxis = FVector::CrossProduct(zaxis, xaxis);
    yaxis.Normalize();

    orientation(0, 0) = xaxis.X;
    orientation(0, 1) = xaxis.Y;
    orientation(0, 2) = xaxis.Z;

    orientation(1, 0) = yaxis.X;
    orientation(1, 1) = yaxis.Y;
    orientation(1, 2) = yaxis.Z;

    orientation(2, 0) = zaxis.X;
    orientation(2, 1) = zaxis.Y;
    orientation(2, 2) = zaxis.Z;

    pos = eye;
}

// +--------------------------------------------------------------------+

void
Camera::LookAt(const FVector& target)
{
    // No navel gazing:
    if (target == Pos())
        return;

    FVector tmp = target - Pos();

    // Rotate into the view orientation:
    FVector tgt;
    tgt.X = (float)(tmp.X * vrt().X + tmp.Y * vrt().Y + tmp.Z * vrt().Z);
    tgt.Y = (float)(tmp.X * vup().X + tmp.Y * vup().Y + tmp.Z * vup().Z);
    tgt.Z = (float)(tmp.X * vpn().X + tmp.Y * vpn().Y + tmp.Z * vpn().Z);

    if (tgt.Z == 0.0f) {
        Pitch(0.5);
        Yaw(0.5);
        LookAt(target);
        return;
    }

    const double az = FMath::Atan2((double)tgt.X, (double)tgt.Z);
    const double el = FMath::Atan2((double)tgt.Y, (double)tgt.Z);

    Pitch(-el);
    Yaw(az);

    // roll to upright position:
    double deflection = (double)vrt().Y;
    while (FMath::Abs(deflection) > 0.001) {
        const double vlen = (double)vrt().Size();
        if (vlen <= 1e-6)
            break;

        const double theta = FMath::Asin(deflection / vlen);
        Roll(-theta);

        deflection = (double)vrt().Y;
    }
}

// +--------------------------------------------------------------------+

bool
Camera::Padlock(const FVector& target, double alimit, double e_lo, double e_hi)
{
    // No navel gazing:
    if (target == Pos())
        return false;

    FVector tmp = target - Pos();

    // Rotate into the view orientation:
    FVector tgt;
    tgt.X = (float)(tmp.X * vrt().X + tmp.Y * vrt().Y + tmp.Z * vrt().Z);
    tgt.Y = (float)(tmp.X * vup().X + tmp.Y * vup().Y + tmp.Z * vup().Z);
    tgt.Z = (float)(tmp.X * vpn().X + tmp.Y * vpn().Y + tmp.Z * vpn().Z);

    if (tgt.Z == 0.0f) {
        Yaw(0.1);

        tgt.X = (float)(tmp.X * vrt().X + tmp.Y * vrt().Y + tmp.Z * vrt().Z);
        tgt.Y = (float)(tmp.X * vup().X + tmp.Y * vup().Y + tmp.Z * vup().Z);
        tgt.Z = (float)(tmp.X * vpn().X + tmp.Y * vpn().Y + tmp.Z * vpn().Z);

        if (tgt.Z == 0.0f)
            return false;
    }

    bool   locked = true;
    double az = FMath::Atan2((double)tgt.X, (double)tgt.Z);

    // Normalize to [-PI, PI]
    while (az > PI) az -= 2.0 * PI;
    while (az < -PI) az += 2.0 * PI;

    if (alimit > 0) {
        if (az < -alimit) {
            az = -alimit;
            locked = false;
        }
        else if (az > alimit) {
            az = alimit;
            locked = false;
        }
    }

    Yaw(az);

    // Rotate into the new view orientation:
    tgt.X = (float)(tmp.X * vrt().X + tmp.Y * vrt().Y + tmp.Z * vrt().Z);
    tgt.Y = (float)(tmp.X * vup().X + tmp.Y * vup().Y + tmp.Z * vup().Z);
    tgt.Z = (float)(tmp.X * vpn().X + tmp.Y * vpn().Y + tmp.Z * vpn().Z);

    double el = FMath::Atan2((double)tgt.Y, (double)tgt.Z);

    if (e_lo > 0 && el < -e_lo) {
        el = -e_lo;
        locked = false;
    }
    else if (e_hi > 0 && el > e_hi) {
        el = e_hi;
        locked = false;
    }

    Pitch(-el);

    return locked;
}

