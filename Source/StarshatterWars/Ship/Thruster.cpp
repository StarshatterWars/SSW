/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Thruster.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Thruster system class.

    Runtime-only migration:
    - No Sprite/Bolt/Graphic rendering
    - No legacy sound ownership
    - Unreal AShipActor owns Niagara and audio
*/

#include "Thruster.h"

#include "Types.h"
#include "Text.h"
#include "List.h"

#include "SimComponent.h"
#include "Drive.h"
#include "FlightComputer.h"
#include "SystemDesign.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

// +----------------------------------------------------------------------+

static int sys_value = 2;

// +----------------------------------------------------------------------+

static int32 ThrusterDirIndex(EThrusterPortDir Dir)
{
    return static_cast<int32>(Dir);
}

// +----------------------------------------------------------------------+

Thruster::Thruster(int dtype, double max_thrust, float flare_scale)
    : SimSystem(
        SYSTEM_CATEGORY::DRIVE,
        dtype,
        "Thruster",
        sys_value,
        max_thrust,
        max_thrust,
        max_thrust),
    ship(nullptr),
    thrust(1.0f),
    scale((flare_scale > 0.0f) ? flare_scale : 1.0f),
    avail_x(1.0f),
    avail_y(1.0f),
    avail_z(1.0f)
{
    name = "Thruster";
    abrv = "Thrust";

    power_flags = POWER_WATTS;

    for (int32 i = 0; i < NumThrusterDirections; ++i)
    {
        burn[i] = 0.0f;
    }

    emcon_power[0] = 50;
    emcon_power[1] = 50;
    emcon_power[2] = 100;
}

// +----------------------------------------------------------------------+

Thruster::Thruster(const Thruster& t)
    : SimSystem(t),
    ship(nullptr),
    thrust(1.0f),
    scale(t.scale),
    avail_x(1.0f),
    avail_y(1.0f),
    avail_z(1.0f)
{
    power_flags = POWER_WATTS;
    Mount(t);

    for (int32 i = 0; i < NumThrusterDirections; ++i)
    {
        burn[i] = 0.0f;
    }

    for (int32 i = 0; i < t.ports.size(); ++i)
    {
        const FThrusterPort* SrcPort = t.ports[i];

        if (!SrcPort)
        {
            continue;
        }

        FThrusterPort* NewPort = new FThrusterPort(*SrcPort);
        NewPort->Burn = 0.0f;

        ports.append(NewPort);
    }
}

// +----------------------------------------------------------------------+

Thruster::~Thruster()
{
    ports.destroy();
}

// +----------------------------------------------------------------------+

void Thruster::ExecFrame(double seconds)
{
    SimSystem::ExecFrame(seconds);

    if (!ship)
    {
        return;
    }

    double rr = 0.0;
    double pr = 0.0;
    double yr = 0.0;

    double rd = 0.0;
    double pd = 0.0;
    double yd = 0.0;

    double agility_factor = 1.0;
    double stability_factor = 1.0;

    FlightComputer* FlightComp = ship->GetFLCS();

    if (FlightComp)
    {
        if (!FlightComp->IsPowerOn() || FlightComp->GetStatus() < SYSTEM_STATUS::DEGRADED)
        {
            agility_factor = 0.3;
            stability_factor = 0.0;
        }
    }

    if (components.size() >= 3)
    {
        SYSTEM_STATUS stat = components[0]->GetStatus();

        if (stat == SYSTEM_STATUS::NOMINAL)
        {
            avail_x = 1.0f;
        }
        else if (stat == SYSTEM_STATUS::DEGRADED)
        {
            avail_x = 0.5f;
        }
        else
        {
            avail_x = 0.0f;
        }

        stat = components[1]->GetStatus();

        if (stat == SYSTEM_STATUS::NOMINAL)
        {
            avail_z = 1.0f;
        }
        else if (stat == SYSTEM_STATUS::DEGRADED)
        {
            avail_z = 0.5f;
        }
        else
        {
            avail_z = 0.0f;
        }

        stat = components[2]->GetStatus();

        if (stat == SYSTEM_STATUS::NOMINAL)
        {
            avail_y = 1.0f;
        }
        else if (stat == SYSTEM_STATUS::DEGRADED)
        {
            avail_y = 0.5f;
        }
        else
        {
            avail_y = 0.0f;
        }
    }

    const float denom = (capacity > 0.0f) ? capacity : 1.0f;

    thrust = energy / denom;
    energy = 0.0f;

    if (thrust < 0.0f)
    {
        thrust = 0.0f;
    }

    agility_factor *= thrust;
    stability_factor *= thrust;

    rr = roll_rate * agility_factor * avail_y;
    pr = pitch_rate * agility_factor * avail_y;
    yr = yaw_rate * agility_factor * avail_x;

    rd = roll_drag * stability_factor * avail_y;
    pd = pitch_drag * stability_factor * avail_y;
    yd = yaw_drag * stability_factor * avail_x;

    ship->SetAngularRates(rr, pr, yr);
    ship->SetAngularDrag(rd, pd, yd);
}

// +----------------------------------------------------------------------+

void Thruster::SetShip(Ship* S)
{
    const double RollSpeed = PI * 0.0400;
    const double PitchSpeed = PI * 0.0250;
    const double YawSpeed = PI * 0.0250;

    ship = S;

    if (!ship)
    {
        return;
    }

    ShipDesign* ShipDesignData = (ShipDesign*)ship->Design();

    if (!ShipDesignData)
    {
        return;
    }

    trans_x = ShipDesignData->trans_x;
    trans_y = ShipDesignData->trans_y;
    trans_z = ShipDesignData->trans_z;

    roll_drag = ShipDesignData->roll_drag;
    pitch_drag = ShipDesignData->pitch_drag;
    yaw_drag = ShipDesignData->yaw_drag;

    roll_rate = (float)(ShipDesignData->roll_rate * PI / 180.0);
    pitch_rate = (float)(ShipDesignData->pitch_rate * PI / 180.0);
    yaw_rate = (float)(ShipDesignData->yaw_rate * PI / 180.0);

    const double Agility = ShipDesignData->agility;

    if (roll_rate == 0.0f)
    {
        roll_rate = (float)(Agility * RollSpeed);
    }

    if (pitch_rate == 0.0f)
    {
        pitch_rate = (float)(Agility * PitchSpeed);
    }

    if (yaw_rate == 0.0f)
    {
        yaw_rate = (float)(Agility * YawSpeed);
    }
}

// +----------------------------------------------------------------------+

double Thruster::TransXLimit()
{
    return trans_x * avail_x;
}

double Thruster::TransYLimit()
{
    return trans_y * avail_y;
}

double Thruster::TransZLimit()
{
    return trans_z * avail_z;
}

// +----------------------------------------------------------------------+

void Thruster::ExecTrans(double x, double y, double z)
{
    if (!ship)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Thruster::ExecTrans] NO SHIP"));
        return;
    }

    if (!ship->Design())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Thruster::ExecTrans] Ship='%hs' Design=NULL"),
            ship->GetName());
        return;
    }

    const double TxLimit =
        FMath::Max(
            ship->Design()->trans_x,
            1.0f);

    const double TyLimit =
        FMath::Max(
            ship->Design()->trans_y,
            1.0f);

    const double TzLimit =
        FMath::Max(
            ship->Design()->trans_z,
            1.0f);

    UE_LOG(LogTemp, Warning,
        TEXT("[Thruster::ExecTrans ENTER] Ship='%hs' "
            "InputLegacy=(X=%.3f Y=%.3f Z=%.3f) "
            "Limits=(X=%.3f Y=%.3f Z=%.3f) "
            "Avail=(X=%.3f Y=%.3f Z=%.3f) "
            "Thrust=%.3f Ports=%d"),
        ship->GetName(),
        x,
        y,
        z,
        TxLimit,
        TyLimit,
        TzLimit,
        avail_x,
        avail_y,
        avail_z,
        thrust,
        ports.size());

    /*
     * IMPORTANT:
     * This is legacy runtime space.
     *
     * x = lateral / right-left
     * y = forward-back
     * z = vertical / up-down
     *
     * Do NOT convert to UE axes here.
     */

    const bool bAirborne =
        ship->IsAirborne();

    const bool bLCA =
        ship->GetClassification() == CLASSIFICATION::LCA;

    if (bLCA &&
        bAirborne &&
        ship->GetVelocity().Length() < 250.0 &&
        ship->GetAltitudeAGL() > ship->GetRadius() / 2.0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Thruster::ExecTrans VTOL] Ship='%hs' "
                "Velocity=%.3f AltAGL=%.3f Radius=%.3f"),
            ship->GetName(),
            ship->GetVelocity().Length(),
            ship->GetAltitudeAGL(),
            ship->GetRadius());

        IncBurn(
            EThrusterPortDir::BOTTOM,
            EThrusterPortDir::TOP);
    }
    else if (!bAirborne)
    {
        /*
         * X axis:
         * +X = right strafe request -> fire LEFT ports
         * -X = left strafe request  -> fire RIGHT ports
         */
        if (x < -0.15 * TxLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS X] Ship='%hs' "
                    "Request=LEFT x=%.3f Threshold=%.3f "
                    "Inc=RIGHT Dec=LEFT"),
                ship->GetName(),
                x,
                -0.15 * TxLimit);

            IncBurn(
                EThrusterPortDir::RIGHT,
                EThrusterPortDir::LEFT);
        }
        else if (x > 0.15 * TxLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS X] Ship='%hs' "
                    "Request=RIGHT x=%.3f Threshold=%.3f "
                    "Inc=LEFT Dec=RIGHT"),
                ship->GetName(),
                x,
                0.15 * TxLimit);

            IncBurn(
                EThrusterPortDir::LEFT,
                EThrusterPortDir::RIGHT);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::LEFT,
                EThrusterPortDir::RIGHT);
        }

        /*
         * Y axis:
         * +Y = forward request -> fire AFT ports
         * -Y = reverse/brake    -> fire FORE ports
         */
        if (y < -0.15 * TyLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS Y] Ship='%hs' "
                    "Request=REVERSE y=%.3f Threshold=%.3f "
                    "Inc=FORE Dec=AFT"),
                ship->GetName(),
                y,
                -0.15 * TyLimit);

            IncBurn(
                EThrusterPortDir::FORE,
                EThrusterPortDir::AFT);
        }
        else if (y > 0.15 * TyLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS Y] Ship='%hs' "
                    "Request=FORWARD y=%.3f Threshold=%.3f "
                    "Inc=AFT Dec=FORE"),
                ship->GetName(),
                y,
                0.15 * TyLimit);

            IncBurn(
                EThrusterPortDir::AFT,
                EThrusterPortDir::FORE);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::FORE,
                EThrusterPortDir::AFT);
        }

        /*
         * Z axis:
         * +Z = up request   -> fire BOTTOM ports
         * -Z = down request -> fire TOP ports
         */
        if (z < -0.15 * TzLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS Z] Ship='%hs' "
                    "Request=DOWN z=%.3f Threshold=%.3f "
                    "Inc=TOP Dec=BOTTOM"),
                ship->GetName(),
                z,
                -0.15 * TzLimit);

            IncBurn(
                EThrusterPortDir::TOP,
                EThrusterPortDir::BOTTOM);
        }
        else if (z > 0.15 * TzLimit)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans AXIS Z] Ship='%hs' "
                    "Request=UP z=%.3f Threshold=%.3f "
                    "Inc=BOTTOM Dec=TOP"),
                ship->GetName(),
                z,
                0.15 * TzLimit);

            IncBurn(
                EThrusterPortDir::BOTTOM,
                EThrusterPortDir::TOP);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::TOP,
                EThrusterPortDir::BOTTOM);
        }

        double Roll = 0.0;
        double Pitch = 0.0;
        double Yaw = 0.0;

        ship->GetAngularThrust(
            Roll,
            Pitch,
            Yaw);

        UE_LOG(LogTemp, Warning,
            TEXT("[Thruster::ExecTrans ANGULAR] Ship='%hs' "
                "Roll=%.3f Pitch=%.3f Yaw=%.3f"),
            ship->GetName(),
            Roll,
            Pitch,
            Yaw);

        if (Roll > 0.0)
        {
            IncBurn(
                EThrusterPortDir::ROLL_L,
                EThrusterPortDir::ROLL_R);
        }
        else if (Roll < 0.0)
        {
            IncBurn(
                EThrusterPortDir::ROLL_R,
                EThrusterPortDir::ROLL_L);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::ROLL_R,
                EThrusterPortDir::ROLL_L);
        }

        if (Yaw < 0.0)
        {
            IncBurn(
                EThrusterPortDir::YAW_L,
                EThrusterPortDir::YAW_R);
        }
        else if (Yaw > 0.0)
        {
            IncBurn(
                EThrusterPortDir::YAW_R,
                EThrusterPortDir::YAW_L);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::YAW_R,
                EThrusterPortDir::YAW_L);
        }

        if (Pitch < 0.0)
        {
            IncBurn(
                EThrusterPortDir::PITCH_D,
                EThrusterPortDir::PITCH_U);
        }
        else if (Pitch > 0.0)
        {
            IncBurn(
                EThrusterPortDir::PITCH_U,
                EThrusterPortDir::PITCH_D);
        }
        else
        {
            DecBurn(
                EThrusterPortDir::PITCH_U,
                EThrusterPortDir::PITCH_D);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Thruster::ExecTrans AIRBORNE DECAY] Ship='%hs' Class=%d"),
            ship->GetName(),
            (int32)ship->GetClassification());

        for (int32 DirIndex = 0;
            DirIndex < NumThrusterDirections;
            ++DirIndex)
        {
            burn[DirIndex] -= 0.1f;

            if (burn[DirIndex] < 0.0f)
            {
                burn[DirIndex] = 0.0f;
            }
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[Thruster::ExecTrans BURN ARRAY] Ship='%hs' "
            "FORE=%.3f AFT=%.3f LEFT=%.3f RIGHT=%.3f "
            "TOP=%.3f BOTTOM=%.3f "
            "ROLL_L=%.3f ROLL_R=%.3f "
            "PITCH_U=%.3f PITCH_D=%.3f "
            "YAW_L=%.3f YAW_R=%.3f"),
        ship->GetName(),
        burn[(int32)EThrusterPortDir::FORE],
        burn[(int32)EThrusterPortDir::AFT],
        burn[(int32)EThrusterPortDir::LEFT],
        burn[(int32)EThrusterPortDir::RIGHT],
        burn[(int32)EThrusterPortDir::TOP],
        burn[(int32)EThrusterPortDir::BOTTOM],
        burn[(int32)EThrusterPortDir::ROLL_L],
        burn[(int32)EThrusterPortDir::ROLL_R],
        burn[(int32)EThrusterPortDir::PITCH_U],
        burn[(int32)EThrusterPortDir::PITCH_D],
        burn[(int32)EThrusterPortDir::YAW_L],
        burn[(int32)EThrusterPortDir::YAW_R]);

    /*
     * Apply burn array to per-port cached Burn.
     */
    for (int32 PortIndex = 0;
        PortIndex < ports.size();
        ++PortIndex)
    {
        FThrusterPort* Port =
            ports[PortIndex];

        if (!Port)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Thruster::ExecTrans PORT] Ship='%hs' Port=%d NULL"),
                ship->GetName(),
                PortIndex);
            continue;
        }

        Port->Burn = 0.0f;

        if (Port->Fire != 0)
        {
            int32 Flag = 1;

            for (int32 DirIndex = 0;
                DirIndex < NumThrusterDirections;
                ++DirIndex)
            {
                if ((Port->Fire & Flag) != 0)
                {
                    const float DirBurn =
                        burn[DirIndex];

                    UE_LOG(LogTemp, Warning,
                        TEXT("[Thruster::ExecTrans MASK HIT] Ship='%hs' "
                            "Port=%d PortDir=%d FireMask=0x%08X "
                            "DirIndex=%d Flag=0x%08X DirBurn=%.3f"),
                        ship->GetName(),
                        PortIndex,
                        (int32)Port->Direction,
                        Port->Fire,
                        DirIndex,
                        Flag,
                        DirBurn);

                    Port->Burn =
                        FMath::Max(
                            Port->Burn,
                            DirBurn);
                }

                Flag <<= 1;
            }
        }
        else
        {
            const int32 DirIndex =
                ThrusterDirIndex(
                    Port->Direction);

            if (DirIndex >= 0 &&
                DirIndex < NumThrusterDirections)
            {
                Port->Burn =
                    burn[DirIndex];

                UE_LOG(LogTemp, Warning,
                    TEXT("[Thruster::ExecTrans DIR DIRECT] Ship='%hs' "
                        "Port=%d PortDir=%d DirIndex=%d DirBurn=%.3f"),
                    ship->GetName(),
                    PortIndex,
                    (int32)Port->Direction,
                    DirIndex,
                    Port->Burn);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("[Thruster::ExecTrans DIR INVALID] Ship='%hs' "
                        "Port=%d PortDir=%d"),
                    ship->GetName(),
                    PortIndex,
                    (int32)Port->Direction);
            }
        }

        Port->Burn =
            FMath::Clamp(
                Port->Burn,
                0.0f,
                1.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("[Thruster::ExecTrans PORT FINAL] Ship='%hs' "
                "Port=%d Dir=%d FireMask=0x%08X Burn=%.3f "
                "Loc=%s Rot=%s PortScale=%.3f"),
            ship->GetName(),
            PortIndex,
            (int32)Port->Direction,
            Port->Fire,
            Port->Burn,
            *Port->Location.ToString(),
            *Port->Rotation.ToString(),
            Port->PortScale);
    }

    /*
     * Preserve legacy runtime behavior:
     * translation request is scaled by actual thruster power.
     */
    const double FinalX =
        x * thrust;

    const double FinalY =
        y * thrust;

    const double FinalZ =
        z * thrust;

    ship->SetTransX(FinalX);
    ship->SetTransY(FinalY);
    ship->SetTransZ(FinalZ);

    UE_LOG(LogTemp, Warning,
        TEXT("[Thruster::ExecTrans EXIT] Ship='%hs' "
            "Input=(%.3f %.3f %.3f) "
            "FinalRequest=(%.3f %.3f %.3f) "
            "ShipTrans=(%.3f %.3f %.3f)"),
        ship->GetName(),
        x,
        y,
        z,
        FinalX,
        FinalY,
        FinalZ,
        ship->GetTransX(),
        ship->GetTransY(),
        ship->GetTransZ());
}
// +----------------------------------------------------------------------+

void Thruster::AddPort(
    EThrusterPortDir Dir,
    const FVector& Loc,
    DWORD Fire,
    float FlareScale)
{
    FThrusterPort* Port = new FThrusterPort();

    Port->Direction = Dir;
    Port->Location = Loc;
    Port->Fire = static_cast<int32>(Fire);

    Port->PortScale = (FlareScale > 0.0f) ? FlareScale : scale;

    Port->FlareScale = Port->PortScale;
    Port->TrailScale = Port->PortScale;

    Port->IntensityMultiplier = 1.0f;
    Port->AudioMultiplier = 1.0f;

    Port->Burn = 0.0f;

    ports.append(Port);
}

// +----------------------------------------------------------------------+

void Thruster::SetPortData(int index, const FThrusterPort& InPort)
{
    if (index < 0 || index >= ports.size())
    {
        return;
    }

    FThrusterPort* Port = ports[index];

    if (!Port)
    {
        return;
    }

    *Port = InPort;
}

// +----------------------------------------------------------------------+

int Thruster::GetNumThrusters() const
{
    return ports.size();
}

// +----------------------------------------------------------------------+

const FThrusterPort* Thruster::GetPort(int index) const
{
    if (index >= 0 && index < ports.size())
    {
        return ports[index];
    }

    return nullptr;
}

// +----------------------------------------------------------------------+

FVector Thruster::GetPortLocation(int index) const
{
    const FThrusterPort* Port = GetPort(index);

    if (Port)
    {
        return Port->Location;
    }

    return FVector::ZeroVector;
}

// +----------------------------------------------------------------------+

FRotator Thruster::GetPortRotation(int index) const
{
    const FThrusterPort* Port = GetPort(index);

    if (Port)
    {
        return Port->Rotation;
    }

    return FRotator::ZeroRotator;
}

// +----------------------------------------------------------------------+

float Thruster::GetPortScale(int index) const
{
    const FThrusterPort* Port = GetPort(index);

    if (Port)
    {
        return Port->PortScale;
    }

    return 1.0f;
}

// +----------------------------------------------------------------------+

float Thruster::GetThrusterBurn(int Index) const
{
	if (Index < 0 || Index >= ports.size())
	{
		return 0.0f;
	}

	const FThrusterPort* Port = ports[Index];

	if (!Port)
	{
		return 0.0f;
	}

	return Port->Burn;
}

// +----------------------------------------------------------------------+

float Thruster::GetVisualPower(int index) const
{
    return FMath::Clamp(GetThrusterBurn(index), 0.0f, 1.0f);
}

// +----------------------------------------------------------------------+

DWORD Thruster::GetPortFireFlags(int index) const
{
    const FThrusterPort* Port = GetPort(index);

    if (!Port)
    {
        return 0;
    }

    return static_cast<DWORD>(Port->Fire);
}

// +----------------------------------------------------------------------+

void Thruster::IncBurn(EThrusterPortDir Inc, EThrusterPortDir Dec)
{
    const int32 IncIndex = ThrusterDirIndex(Inc);
    const int32 DecIndex = ThrusterDirIndex(Dec);

    if (IncIndex >= 0 && IncIndex < NumThrusterDirections)
    {
        burn[IncIndex] += 0.1f;

        if (burn[IncIndex] > 1.0f)
        {
            burn[IncIndex] = 1.0f;
        }
    }

    if (DecIndex >= 0 && DecIndex < NumThrusterDirections)
    {
        burn[DecIndex] -= 0.1f;

        if (burn[DecIndex] < 0.0f)
        {
            burn[DecIndex] = 0.0f;
        }
    }
}

// +----------------------------------------------------------------------+

void Thruster::DecBurn(EThrusterPortDir A, EThrusterPortDir B)
{
    const int32 AIndex = ThrusterDirIndex(A);
    const int32 BIndex = ThrusterDirIndex(B);

    if (AIndex >= 0 && AIndex < NumThrusterDirections)
    {
        burn[AIndex] -= 0.1f;

        if (burn[AIndex] < 0.0f)
        {
            burn[AIndex] = 0.0f;
        }
    }

    if (BIndex >= 0 && BIndex < NumThrusterDirections)
    {
        burn[BIndex] -= 0.1f;

        if (burn[BIndex] < 0.0f)
        {
            burn[BIndex] = 0.0f;
        }
    }
}

// +----------------------------------------------------------------------+

double Thruster::GetRequest(double seconds) const
{
    if (!power_on)
    {
        return 0.0;
    }

    for (int32 i = 0; i < NumThrusterDirections; ++i)
    {
        if (burn[i] != 0.0f)
        {
            return power_level * sink_rate * seconds;
        }
    }

    return 0.0;
}