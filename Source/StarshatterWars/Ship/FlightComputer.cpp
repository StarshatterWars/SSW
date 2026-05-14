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
    , mode(EFLCSMode::AUTO)
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
    , mode(EFLCSMode::AUTO)
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
    const double Tx =
        ship->GetTransX();

    const double Ty =
        ship->GetTransY();

    const double Tz =
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
    // AUTO MODE
    //-------------------------------------------------------------
    if (mode == EFLCSMode::AUTO)
    {
        //---------------------------------------------------------
        // Side stabilization
        //---------------------------------------------------------
        if (FMath::IsNearlyZero(Tx))
        {
            if (bFlcsOperative)
            {
                TransX =
                    SideVel * -200.0;
            }
            else
            {
                TransX = 0.0;
            }
        }
        else
        {
            if (FMath::Abs(SideVel) >= vlimit)
            {
                if (TransX > 0.0 && SideVel > 0.0)
                {
                    TransX = 0.0;
                }
                else if (TransX < 0.0 && SideVel < 0.0)
                {
                    TransX = 0.0;
                }
            }
        }

        //---------------------------------------------------------
        // Halt mode
        //---------------------------------------------------------
        if (halt && bFlcsOperative)
        {
            if (FMath::IsNearlyZero(Ty))
            {
                const double VMag =
                    FMath::Abs(ForwardVel);

                if (VMag > 0.0)
                {
                    if (ForwardVel > 0.0)
                    {
                        TransY = -trans_y_limit;
                    }
                    else
                    {
                        TransY = trans_y_limit;
                    }

                    if (ForwardVel < vlimit / 2.0)
                    {
                        TransY *=
                            (VMag / (vlimit / 2.0));
                    }
                }
            }
        }

        //---------------------------------------------------------
        // Vertical stabilization
        //---------------------------------------------------------
        if (FMath::IsNearlyZero(Tz))
        {
            if (bFlcsOperative)
            {
                TransZ =
                    UpVel * -200.0;
            }
            else
            {
                TransZ = 0.0;
            }
        }
        else
        {
            if (FMath::Abs(UpVel) >= vlimit)
            {
                if (TransZ > 0.0 && UpVel > 0.0)
                {
                    TransZ = 0.0;
                }
                else if (TransZ < 0.0 && UpVel < 0.0)
                {
                    TransZ = 0.0;
                }
            }
        }
    }

    //-------------------------------------------------------------
    // STARSHIP HELM MODE
    //-------------------------------------------------------------
    else if (mode == EFLCSMode::HELM)
    {
        //---------------------------------------------------------
        // Helm stabilization
        //---------------------------------------------------------
        if (bFlcsOperative)
        {
            const double CompassHeading =
                ship->GetCompassHeading();

            const double CompassPitch =
                ship->GetCompassPitch();

            //-----------------------------------------------------
            // Rotate helm into compass orientation
            //-----------------------------------------------------
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

            //-----------------------------------------------------
            // LEGACY:
            // rotate actual ship toward helm heading
            //-----------------------------------------------------
            if (!FMath::IsNearlyZero(HelmError))
            {
                ship->ApplyYaw(HelmError);
            }

            //-----------------------------------------------------
            // LEGACY:
            // rotate actual ship pitch toward helm pitch
            //-----------------------------------------------------
            if (CompassPitch != ship->GetHelmPitch())
            {
                ship->ApplyPitch(
                    CompassPitch -
                    ship->GetHelmPitch());
            }

            //-----------------------------------------------------
            // LEGACY AUTO ROLL
            //-----------------------------------------------------
            if (ship->Design() &&
                ship->Design()->auto_roll > 0)
            {
                const FVector VRT =
                    ship->GetCam().vrt();

                const double Deflection =
                    VRT.Y;

                if (FMath::Abs(HelmError) < UE_PI / 16.0 ||
                    ship->Design()->turn_bank < 0.01)
                {
                    if (ship->Design()->auto_roll > 1)
                    {
                        ship->ApplyRoll(0.5);
                    }
                    else if (!FMath::IsNearlyZero(Deflection))
                    {
                        const double Theta =
                            FMath::Asin(Deflection);

                        ship->ApplyRoll(-Theta);
                    }
                }
                else
                {
                    double DesiredBank =
                        ship->Design()->turn_bank;

                    if (HelmError >= 0.0)
                    {
                        DesiredBank =
                            -DesiredBank;
                    }

                    const double CurrentBank =
                        FMath::Asin(Deflection);

                    const double Theta =
                        DesiredBank -
                        CurrentBank;

                    ship->ApplyRoll(Theta);

                    //-------------------------------------------------
                    // Coordinated turn
                    //-------------------------------------------------
                    if ((CurrentBank < 0.0 && DesiredBank < 0.0) ||
                        (CurrentBank > 0.0 && DesiredBank > 0.0))
                    {
                        const double CoordPitch =
                            CompassPitch -
                            ship->GetHelmPitch() -
                            FMath::Abs(HelmError) *
                            FMath::Abs(CurrentBank);

                        ship->ApplyPitch(CoordPitch);
                    }
                }
            }
        }
        else
        {
            //-----------------------------------------------------
            // FLCS inoperative
            //-----------------------------------------------------
            ship->SetHelmHeading(
                ship->GetCompassHeading());

            ship->SetHelmPitch(
                ship->GetCompassPitch());
        }

        //---------------------------------------------------------
        // Side stabilization
        //---------------------------------------------------------
        if (FMath::IsNearlyZero(Tx))
        {
            if (bFlcsOperative)
            {
                TransX =
                    SideVel *
                    ship->GetMass() *
                    -1.0;
            }
            else
            {
                TransX = 0.0;
            }
        }
        else
        {
            if (FMath::Abs(SideVel) >= vlimit / 2.0)
            {
                if (TransX > 0.0 && SideVel > 0.0)
                {
                    TransX = 0.0;
                }
                else if (TransX < 0.0 && SideVel < 0.0)
                {
                    TransX = 0.0;
                }
            }
        }

        //---------------------------------------------------------
        // Forward halt mode
        //---------------------------------------------------------
        if (FMath::IsNearlyZero(TransY) && halt)
        {
            const double DesiredVel = 0.0;

            if (ForwardVel > DesiredVel)
            {
                TransY = -trans_y_limit;

                if (!bFlcsOperative)
                {
                    TransY = 0.0;
                }

                const double Delta =
                    ForwardVel - DesiredVel;

                if (Delta < vlimit / 2.0)
                {
                    TransY *=
                        (Delta / (vlimit / 2.0));
                }
            }
        }

        //---------------------------------------------------------
        // Vertical stabilization
        //---------------------------------------------------------
        if (FMath::IsNearlyZero(Tz))
        {
            if (bFlcsOperative)
            {
                TransZ =
                    UpVel *
                    ship->GetMass() *
                    -1.0;
            }
            else
            {
                TransZ = 0.0;
            }
        }
        else
        {
            if (FMath::Abs(UpVel) > vlimit / 2.0)
            {
                if (TransZ > 0.0 && UpVel > 0.0)
                {
                    TransZ = 0.0;
                }
                else if (TransZ < 0.0 && UpVel < 0.0)
                {
                    TransZ = 0.0;
                }
            }
        }
    }

    //-------------------------------------------------------------
    // Final translational authority
    //-------------------------------------------------------------
    if (ship->GetThruster())
    {
        ship->GetThruster()->ExecTrans(
            TransX,
            TransY,
            TransZ);
    }
    else
    {
        ship->SetTransX(TransX);
        ship->SetTransY(TransY);
        ship->SetTransZ(TransZ);
    }
}