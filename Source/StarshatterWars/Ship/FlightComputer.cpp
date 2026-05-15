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
    if (ship)
    {
        ExecThrottle();
        ExecTrans();
    }
}


// +--------------------------------------------------------------------+

void
FlightComputer::ExecThrottle()
{
    throttle =
        (float)ship->GetThrottle();

    if (throttle > 5.0f)
    {
        halt =
            false;
    }
}
// +--------------------------------------------------------------------+

void
FlightComputer::ExecTrans()
{
	double tx =
		ship->GetTransX();

	double ty =
		ship->GetTransY();

	double tz =
		ship->GetTransZ();

	double trans_x =
		tx;

	double trans_y =
		ty;

	double trans_z =
		tz;

	bool flcs_operative =
		false;

	if (IsPowerOn())
	{
		flcs_operative =
			GetStatus() == SYSTEM_STATUS::NOMINAL ||
			GetStatus() == SYSTEM_STATUS::DEGRADED;
	}

	//-------------------------------------------------------------
	// FIGHTER FLCS AUTO MODE
	//-------------------------------------------------------------
	if (mode == EFLCSMode::AUTO)
	{
		if (tx == 0.0)
		{
			if (flcs_operative)
			{
				trans_x =
					FVector::DotProduct(
						ship->GetVelocity(),
						ship->GetBeamLine()) * -200.0;
			}
			else
			{
				trans_x =
					0.0;
			}
		}
		else
		{
			const double vfwd =
				FVector::DotProduct(
					ship->GetBeamLine(),
					ship->GetVelocity());

			if (FMath::Abs(vfwd) >= vlimit)
			{
				if (trans_x > 0.0 &&
					vfwd > 0.0)
				{
					trans_x =
						0.0;
				}
				else if (trans_x < 0.0 &&
					vfwd < 0.0)
				{
					trans_x =
						0.0;
				}
			}
		}

		if (halt &&
			flcs_operative)
		{
			if (ty == 0.0)
			{
				const double vfwd =
					FVector::DotProduct(
						ship->GetHeading(),
						ship->GetVelocity());

				const double vmag =
					FMath::Abs(vfwd);

				if (vmag > 0.0)
				{
					if (vfwd > 0.0)
					{
						trans_y =
							-trans_y_limit;
					}
					else
					{
						trans_y =
							trans_y_limit;
					}

					if (vfwd < vlimit / 2.0)
					{
						trans_y *=
							vmag / (vlimit / 2.0);
					}
				}
			}
		}

		if (tz == 0.0)
		{
			if (flcs_operative)
			{
				trans_z =
					FVector::DotProduct(
						ship->GetVelocity(),
						ship->GetLiftLine()) * -200.0;
			}
			else
			{
				trans_z =
					0.0;
			}
		}
		else
		{
			const double vfwd =
				FVector::DotProduct(
					ship->GetLiftLine(),
					ship->GetVelocity());

			if (FMath::Abs(vfwd) >= vlimit)
			{
				if (trans_z > 0.0 &&
					vfwd > 0.0)
				{
					trans_z =
						0.0;
				}
				else if (trans_z < 0.0 &&
					vfwd < 0.0)
				{
					trans_z =
						0.0;
				}
			}
		}
	}

	//-------------------------------------------------------------
	// STARSHIP HELM MODE
	//-------------------------------------------------------------
	else if (mode == EFLCSMode::HELM)
	{
		if (flcs_operative)
		{
			const double compass_heading =
				ship->GetCompassHeading();

			const double compass_pitch =
				ship->GetCompassPitch();

			double helm =
				ship->GetHelmHeading() -
				compass_heading;

			if (helm > PI)
			{
				helm -=
					2.0 * PI;
			}
			else if (helm < -PI)
			{
				helm +=
					2.0 * PI;
			}

			if (helm != 0.0)
			{
				ship->ApplyYaw(
					helm);
			}

			if (compass_pitch != ship->GetHelmPitch())
			{
				ship->ApplyPitch(
					compass_pitch -
					ship->GetHelmPitch());
			}

			if (ship->Design() &&
				ship->Design()->auto_roll > 0)
			{
				const FVector vrt =
					ship->GetCam().vrt();

				const double deflection =
					vrt.Y;

				if (FMath::Abs(helm) < PI / 16.0 ||
					ship->Design()->turn_bank < 0.01)
				{
					if (ship->Design()->auto_roll > 1)
					{
						ship->ApplyRoll(
							0.5);
					}
					else if (deflection != 0.0)
					{
						const double theta =
							FMath::Asin(deflection);

						ship->ApplyRoll(
							-theta);
					}
				}
				else
				{
					double desired_bank =
						ship->Design()->turn_bank;

					if (helm >= 0.0)
					{
						desired_bank =
							-desired_bank;
					}

					const double current_bank =
						FMath::Asin(deflection);

					const double theta =
						desired_bank -
						current_bank;

					ship->ApplyRoll(
						theta);

					if ((current_bank < 0.0 && desired_bank < 0.0) ||
						(current_bank > 0.0 && desired_bank > 0.0))
					{
						const double coord_pitch =
							compass_pitch -
							ship->GetHelmPitch() -
							FMath::Abs(helm) *
							FMath::Abs(current_bank);

						ship->ApplyPitch(
							coord_pitch);
					}
				}
			}
		}
		else
		{
			ship->SetHelmHeading(
				ship->GetCompassHeading());

			ship->SetHelmPitch(
				ship->GetCompassPitch());
		}

		if (tx == 0.0)
		{
			if (flcs_operative)
			{
				trans_x =
					FVector::DotProduct(
						ship->GetVelocity(),
						ship->GetBeamLine()) *
					ship->GetMass() *
					-1.0;
			}
			else
			{
				trans_x =
					0.0;
			}
		}
		else
		{
			const double vfwd =
				FVector::DotProduct(
					ship->GetBeamLine(),
					ship->GetVelocity());

			if (FMath::Abs(vfwd) >= vlimit / 2.0)
			{
				if (trans_x > 0.0 &&
					vfwd > 0.0)
				{
					trans_x =
						0.0;
				}
				else if (trans_x < 0.0 &&
					vfwd < 0.0)
				{
					trans_x =
						0.0;
				}
			}
		}

		if (trans_y == 0.0 &&
			halt)
		{
			const double vfwd =
				FVector::DotProduct(
					ship->GetHeading(),
					ship->GetVelocity());

			const double vdesired =
				0.0;

			if (vfwd > vdesired)
			{
				trans_y =
					-trans_y_limit;

				if (!flcs_operative)
				{
					trans_y =
						0.0;
				}

				const double vdelta =
					vfwd -
					vdesired;

				if (vdelta < vlimit / 2.0)
				{
					trans_y *=
						vdelta / (vlimit / 2.0);
				}
			}
		}

		if (tz == 0.0)
		{
			if (flcs_operative)
			{
				trans_z =
					FVector::DotProduct(
						ship->GetVelocity(),
						ship->GetLiftLine()) *
					ship->GetMass() *
					-1.0;
			}
			else
			{
				trans_z =
					0.0;
			}
		}
		else
		{
			const double vfwd =
				FVector::DotProduct(
					ship->GetLiftLine(),
					ship->GetVelocity());

			if (FMath::Abs(vfwd) > vlimit / 2.0)
			{
				if (trans_z > 0.0 &&
					vfwd > 0.0)
				{
					trans_z =
						0.0;
				}
				else if (trans_z < 0.0 &&
					vfwd < 0.0)
				{
					trans_z =
						0.0;
				}
			}
		}
	}

	if (ship->GetThruster())
	{
		ship->GetThruster()->ExecTrans(
			trans_x,
			trans_y,
			trans_z);
	}
	else
	{
		ship->SetTransX(
			trans_x);

		ship->SetTransY(
			trans_y);

		ship->SetTransZ(
			trans_z);
	}
}