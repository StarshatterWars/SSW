/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         FlightComputer.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Flight Computer System class
*/

#include "FlightComputer.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "Ship.h"
#include "ShipDesign.h"
#include "Thruster.h"

// +----------------------------------------------------------------------+

FlightComputer::FlightComputer(int comp_type, const char* comp_name)
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

// +----------------------------------------------------------------------+

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
FlightComputer::SetTransLimit(double x, double y, double z)
{
	trans_x_limit = 0.0f;
	trans_y_limit = 0.0f;
	trans_z_limit = 0.0f;

	if (x >= 0) trans_x_limit = (float)x;
	if (y >= 0) trans_y_limit = (float)y;
	if (z >= 0) trans_z_limit = (float)z;
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecSubFrame()
{
	if (ship) {
		ExecThrottle();
		ExecTrans();
	}
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecThrottle()
{
	throttle = (float)ship->Throttle();

	if (throttle > 5)
		halt = false;
}

// +--------------------------------------------------------------------+

void
FlightComputer::ExecTrans()
{
	double Tx = ship->TransX();
	double Ty = ship->TransY();
	double Tz = ship->TransZ();

	double TransX = Tx;
	double TransY = Ty;
	double TransZ = Tz;

	bool bFlcsOperative = false;

	if (IsPowerOn())
	{
		bFlcsOperative =
			GetStatus() == SYSTEM_STATUS::NOMINAL ||
			GetStatus() == SYSTEM_STATUS::DEGRADED;
	}

	// Convenience: Starshatter-style "*" was effectively dot-product.
	const FVector Vel = ship->Velocity();
	const FVector Beam = ship->BeamLine();
	const FVector Lift = ship->LiftLine();
	const FVector Head = ship->Heading();

	// ----------------------------------------------------------
	// FIGHTER FLCS AUTO MODE
	// ----------------------------------------------------------
	if (mode == Ship::FLCS_AUTO)
	{
		// auto thrust to align flight path with orientation:
		if (FMath::IsNearlyZero(Tx))
		{
			if (bFlcsOperative)
				TransX = FVector::DotProduct(Vel, Beam) * -200.0;
			else
				TransX = 0.0;
		}

		// manual thrust up to vlimit:
		else
		{
			const double Vfwd = FVector::DotProduct(Beam, Vel);

			if (FMath::Abs(Vfwd) >= vlimit)
			{
				if (TransX > 0.0 && Vfwd > 0.0)
					TransX = 0.0;
				else if (TransX < 0.0 && Vfwd < 0.0)
					TransX = 0.0;
			}
		}

		if (halt && bFlcsOperative)
		{
			if (FMath::IsNearlyZero(Ty))
			{
				const double Vfwd = FVector::DotProduct(Head, Vel);
				const double Vmag = FMath::Abs(Vfwd);

				if (Vmag > 0.0)
				{
					TransY = (Vfwd > 0.0) ? -trans_y_limit : trans_y_limit;

					if (Vfwd < vlimit / 2.0)
						TransY *= (Vmag / (vlimit / 2.0));
				}
			}
		}

		// auto thrust to align flight path with orientation:
		if (FMath::IsNearlyZero(Tz))
		{
			if (bFlcsOperative)
				TransZ = FVector::DotProduct(Vel, Lift) * -200.0;
			else
				TransZ = 0.0;
		}

		// manual thrust up to vlimit:
		else
		{
			const double Vfwd = FVector::DotProduct(Lift, Vel);

			if (FMath::Abs(Vfwd) >= vlimit)
			{
				if (TransZ > 0.0 && Vfwd > 0.0)
					TransZ = 0.0;
				else if (TransZ < 0.0 && Vfwd < 0.0)
					TransZ = 0.0;
			}
		}
	}

	// ----------------------------------------------------------
	// STARSHIP HELM MODE
	// ----------------------------------------------------------
	else if (mode == Ship::FLCS_HELM)
	{
		if (bFlcsOperative)
		{
			const double CompassHeading = ship->CompassHeading();
			const double CompassPitch = ship->CompassPitch();

			// rotate helm into compass orientation:
			double Helm = ship->GetHelmHeading() - CompassHeading;

			if (Helm > UE_PI)
				Helm -= 2.0 * UE_PI;
			else if (Helm < -UE_PI)
				Helm += 2.0 * UE_PI;

			// turn to align with helm heading:
			if (!FMath::IsNearlyZero(Helm))
				ship->ApplyYaw(Helm);

			// pitch to align with helm pitch:
			if (!FMath::IsNearlyEqual(CompassPitch, ship->GetHelmPitch()))
				ship->ApplyPitch(CompassPitch - ship->GetHelmPitch());

			// roll to align with world coordinates:
			if (ship->Design()->auto_roll > 0)
			{
				// Ensure ship->Cam().vrt() is already an FVector (preferred).
				// If it's a legacy Vec3, add a conversion helper and use it here.
				const FVector Vrt = ship->Cam().vrt();

				// Starshatter used Y as "deflection" here; keep as-is:
				const double Deflection = Vrt.Y;

				if (FMath::Abs(Helm) < UE_PI / 16.0 || ship->Design()->turn_bank < 0.01)
				{
					if (ship->Design()->auto_roll > 1)
					{
						ship->ApplyRoll(0.5);
					}
					else if (!FMath::IsNearlyZero(Deflection))
					{
						const double Theta = FMath::Asin(Deflection);
						ship->ApplyRoll(-Theta);
					}
				}

				// else roll through turn maneuvers:
				else
				{
					double DesiredBank = ship->Design()->turn_bank;
					if (Helm >= 0.0)
						DesiredBank = -DesiredBank;

					const double CurrentBank = FMath::Asin(Deflection);
					const double Theta = DesiredBank - CurrentBank;
					ship->ApplyRoll(Theta);

					// coordinate the turn:
					if ((CurrentBank < 0.0 && DesiredBank < 0.0) ||
						(CurrentBank > 0.0 && DesiredBank > 0.0))
					{
						const double CoordPitch =
							CompassPitch
							- ship->GetHelmPitch()
							- FMath::Abs(Helm) * FMath::Abs(CurrentBank);

						ship->ApplyPitch(CoordPitch);
					}
				}
			}
		}

		// flcs inoperative, set helm heading based on actual compass heading:
		else
		{
			ship->SetHelmHeading(ship->CompassHeading());
			ship->SetHelmPitch(ship->CompassPitch());
		}

		// auto thrust to align flight path with helm order:
		if (FMath::IsNearlyZero(Tx))
		{
			if (bFlcsOperative)
				TransX = FVector::DotProduct(Vel, Beam) * ship->Mass() * -1.0;
			else
				TransX = 0.0;
		}

		// manual thrust up to vlimit/2:
		else
		{
			const double Vfwd = FVector::DotProduct(Beam, Vel);

			if (FMath::Abs(Vfwd) >= vlimit / 2.0)
			{
				if (TransX > 0.0 && Vfwd > 0.0)
					TransX = 0.0;
				else if (TransX < 0.0 && Vfwd < 0.0)
					TransX = 0.0;
			}
		}

		if (FMath::IsNearlyZero(TransY) && halt)
		{
			const double Vfwd = FVector::DotProduct(Head, Vel);
			const double Vdesired = 0.0;

			if (Vfwd > Vdesired)
			{
				TransY = -trans_y_limit;

				if (!bFlcsOperative)
					TransY = 0.0;

				const double Vdelta = Vfwd - Vdesired;
				if (Vdelta < vlimit / 2.0)
					TransY *= (Vdelta / (vlimit / 2.0));
			}
		}

		// auto thrust to align flight path with helm order:
		if (FMath::IsNearlyZero(Tz))
		{
			if (bFlcsOperative)
				TransZ = FVector::DotProduct(Vel, Lift) * ship->Mass() * -1.0;
			else
				TransZ = 0.0;
		}

		// manual thrust up to vlimit/2:
		else
		{
			const double Vfwd = FVector::DotProduct(Lift, Vel);

			if (FMath::Abs(Vfwd) > vlimit / 2.0)
			{
				if (TransZ > 0.0 && Vfwd > 0.0)
					TransZ = 0.0;
				else if (TransZ < 0.0 && Vfwd < 0.0)
					TransZ = 0.0;
			}
		}
	}

	// Apply translation either through thruster subsystem or directly to ship:
	if (ship->GetThruster())
	{
		ship->GetThruster()->ExecTrans(TransX, TransY, TransZ);
	}
	else
	{
		ship->SetTransX(TransX);
		ship->SetTransY(TransY);
		ship->SetTransZ(TransZ);
	}
}
