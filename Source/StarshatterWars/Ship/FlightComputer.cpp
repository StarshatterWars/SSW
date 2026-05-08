/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         FlightComputer.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Unreal-native Flight Computer System
*/

#include "FlightComputer.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "Ship.h"
#include "ShipDesign.h"
#include "Thruster.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

FlightComputer::FlightComputer(
    EComputerType comp_type,
    const char* comp_name)
    : Computer(comp_type, comp_name)
    , mode(0)
    , halt(0)
    , throttle(0.0f)
    , vlimit(0.0f)
    , trans_x_limit(0.0f)
    , trans_y_limit(0.0f)
    , trans_z_limit(0.0f)
{
}

// +--------------------------------------------------------------------+

FlightComputer::FlightComputer(const Computer& c)
    : Computer(c)
    , mode(0)
    , halt(0)
    , throttle(0.0f)
    , vlimit(0.0f)
    , trans_x_limit(0.0f)
    , trans_y_limit(0.0f)
    , trans_z_limit(0.0f)
{
}

// +--------------------------------------------------------------------+

FlightComputer::~FlightComputer()
{
}

// +--------------------------------------------------------------------+

void
FlightComputer::SetTransLimit(
    double x,
    double y,
    double z)
{
    trans_x_limit = 0.0f;
    trans_y_limit = 0.0f;
    trans_z_limit = 0.0f;

    if (x >= 0.0)
    {
        trans_x_limit = (float)x;
    }

    if (y >= 0.0)
    {
        trans_y_limit = (float)y;
    }

    if (z >= 0.0)
    {
        trans_z_limit = (float)z;
    }
}

// +--------------------------------------------------------------------+

FVector
FlightComputer::GetForwardVector() const
{
    return ship ?
        ship->GetHeading().GetSafeNormal() :
        FVector::ForwardVector;
}

// +--------------------------------------------------------------------+

FVector
FlightComputer::GetRightVector() const
{
    return ship ?
        ship->GetBeamLine().GetSafeNormal() :
        FVector::RightVector;
}

// +--------------------------------------------------------------------+

FVector
FlightComputer::GetUpVector() const
{
    return ship ?
        ship->GetLiftLine().GetSafeNormal() :
        FVector::UpVector;
}

// +--------------------------------------------------------------------+

double
FlightComputer::GetForwardVelocity() const
{
    if (!ship)
    {
        return 0.0;
    }

    return FVector::DotProduct(
        ship->GetVelocity(),
        GetForwardVector());
}

// +--------------------------------------------------------------------+

double
FlightComputer::GetSideVelocity() const
{
    if (!ship)
    {
        return 0.0;
    }

    return FVector::DotProduct(
        ship->GetVelocity(),
        GetRightVector());
}

// +--------------------------------------------------------------------+

double
FlightComputer::GetVerticalVelocity() const
{
    if (!ship)
    {
        return 0.0;
    }

    return FVector::DotProduct(
        ship->GetVelocity(),
        GetUpVector());
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecSubFrame()
{
    if (!ship)
    {
        return;
    }

    ExecThrottle();
    ExecTrans();
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecThrottle()
{
    throttle = (float)ship->GetThrottle();

    if (throttle > 5.0f)
    {
        halt = false;
    }
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecTrans()
{
    if (!ship)
    {
        return;
    }

    //-------------------------------------------------------------
    // FLCS operational state
    //-------------------------------------------------------------
    const bool bFlcsOperative =
        IsPowerOn() &&
        (GetStatus() == SYSTEM_STATUS::NOMINAL ||
            GetStatus() == SYSTEM_STATUS::DEGRADED);

    //-------------------------------------------------------------
    // Pilot input state
    //-------------------------------------------------------------
    double Tx =
        ship->GetTransX();

    double Ty =
        ship->GetTransY();

    double Tz =
        ship->GetTransZ();

    double TransX = Tx;
    double TransY = Ty;
    double TransZ = Tz;

    //-------------------------------------------------------------
    // Ship orientation
    //-------------------------------------------------------------
    const FVector Velocity =
        ship->GetVelocity();

    const FVector Forward =
        ship->GetHeading().GetSafeNormal();

    const FVector Right =
        ship->GetBeamLine().GetSafeNormal();

    const FVector Up =
        ship->GetLiftLine().GetSafeNormal();

    //-------------------------------------------------------------
    // Local-space velocity
    //-------------------------------------------------------------
    const double ForwardVel =
        FVector::DotProduct(
            Velocity,
            Forward);

    const double SideVel =
        FVector::DotProduct(
            Velocity,
            Right);

    const double UpVel =
        FVector::DotProduct(
            Velocity,
            Up);

    //-------------------------------------------------------------
    // Unreal-native FLCS stabilization
    //-------------------------------------------------------------
    if (bFlcsOperative)
    {
        constexpr double DriftDamping = 0.25;

        /*
         * Side drift correction
         */
        if (FMath::IsNearlyZero(Tx))
        {
            TransX =
                FMath::Clamp(
                    -SideVel * DriftDamping,
                    -trans_x_limit,
                    trans_x_limit);
        }

        /*
         * Vertical drift correction
         */
        if (FMath::IsNearlyZero(Tz))
        {
            TransZ =
                FMath::Clamp(
                    -UpVel * DriftDamping,
                    -trans_z_limit,
                    trans_z_limit);
        }

        /*
         * Halt mode
         */
        if (halt &&
            FMath::IsNearlyZero(Ty))
        {
            constexpr double ForwardDamping = 0.5;

            TransY =
                FMath::Clamp(
                    -ForwardVel * ForwardDamping,
                    -trans_y_limit,
                    trans_y_limit);
        }
    }

    //-------------------------------------------------------------
    // Helm stabilization
    //-------------------------------------------------------------
    if (mode == Ship::FLCS_HELM &&
        bFlcsOperative)
    {
        const double CompassHeading =
            ship->GetCompassHeading();

        const double CompassPitch =
            ship->GetCompassPitch();

        double HelmError =
            ship->GetHelmHeading() -
            CompassHeading;

        if (HelmError > UE_PI)
        {
            HelmError -= UE_TWO_PI;
        }
        else if (HelmError < -UE_PI)
        {
            HelmError += UE_TWO_PI;
        }

        if (!FMath::IsNearlyZero(HelmError))
        {
            ship->ApplyYaw(HelmError);
        }

        const double PitchError =
            ship->GetHelmPitch() -
            CompassPitch;

        if (!FMath::IsNearlyZero(PitchError))
        {
            ship->ApplyPitch(PitchError);
        }
    }

    //-------------------------------------------------------------
    // Final translational authority
    //-------------------------------------------------------------
    ship->SetTransX(TransX);
    ship->SetTransY(TransY);
    ship->SetTransZ(TransZ);

    //-------------------------------------------------------------
    // Thruster FX / burn state
    //-------------------------------------------------------------
    if (ship->GetThruster())
    {
        ship->GetThruster()->ExecTrans(
            TransX,
            TransY,
            TransZ);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[FlightComputer::ExecTrans FINAL] "
            "Ship='%s' "
            "ForwardVel=%.3f "
            "SideVel=%.3f "
            "UpVel=%.3f "
            "Final=(%.3f %.3f %.3f) "
            "Limits=(%.3f %.3f %.3f)"),
        ANSI_TO_TCHAR(ship->GetName()),
        ForwardVel,
        SideVel,
        UpVel,
        TransX,
        TransY,
        TransZ,
        trans_x_limit,
        trans_y_limit,
        trans_z_limit);
}