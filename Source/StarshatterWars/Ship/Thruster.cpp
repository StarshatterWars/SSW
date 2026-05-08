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

    FlightComputer* flcs = ship->GetFLCS();

    if (flcs)
    {
        if (!flcs->IsPowerOn() || flcs->GetStatus() < SYSTEM_STATUS::DEGRADED)
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
        return;
    }

    if (ship->Class() == CLASSIFICATION::LCA &&
        ship->IsAirborne() &&
        ship->GetVelocity().Length() < 250 &&
        ship->GetAltitudeAGL() > ship->GetRadius() / 2)
    {
        IncBurn(EThrusterPortDir::BOTTOM, EThrusterPortDir::TOP);
    }
    else if (!ship->IsAirborne())
    {
        const double tx_limit = ship->Design()->trans_x;
        const double ty_limit = ship->Design()->trans_y;
        const double tz_limit = ship->Design()->trans_z;

        if (x < -0.15 * tx_limit)
        {
            IncBurn(EThrusterPortDir::RIGHT, EThrusterPortDir::LEFT);
        }
        else if (x > 0.15 * tx_limit)
        {
            IncBurn(EThrusterPortDir::LEFT, EThrusterPortDir::RIGHT);
        }
        else
        {
            DecBurn(EThrusterPortDir::LEFT, EThrusterPortDir::RIGHT);
        }

        if (y < -0.15 * ty_limit)
        {
            IncBurn(EThrusterPortDir::FORE, EThrusterPortDir::AFT);
        }
        else if (y > 0.15 * ty_limit)
        {
            IncBurn(EThrusterPortDir::AFT, EThrusterPortDir::FORE);
        }
        else
        {
            DecBurn(EThrusterPortDir::FORE, EThrusterPortDir::AFT);
        }

        if (z < -0.15 * tz_limit)
        {
            IncBurn(EThrusterPortDir::TOP, EThrusterPortDir::BOTTOM);
        }
        else if (z > 0.15 * tz_limit)
        {
            IncBurn(EThrusterPortDir::BOTTOM, EThrusterPortDir::TOP);
        }
        else
        {
            DecBurn(EThrusterPortDir::TOP, EThrusterPortDir::BOTTOM);
        }

        double r = 0.0;
        double p = 0.0;
        double yaw = 0.0;

        ship->GetAngularThrust(r, p, yaw);

        if (r > 0.0)
        {
            IncBurn(EThrusterPortDir::ROLL_L, EThrusterPortDir::ROLL_R);
        }
        else if (r < 0.0)
        {
            IncBurn(EThrusterPortDir::ROLL_R, EThrusterPortDir::ROLL_L);
        }
        else
        {
            DecBurn(EThrusterPortDir::ROLL_R, EThrusterPortDir::ROLL_L);
        }

        if (yaw < 0.0)
        {
            IncBurn(EThrusterPortDir::YAW_L, EThrusterPortDir::YAW_R);
        }
        else if (yaw > 0.0)
        {
            IncBurn(EThrusterPortDir::YAW_R, EThrusterPortDir::YAW_L);
        }
        else
        {
            DecBurn(EThrusterPortDir::YAW_R, EThrusterPortDir::YAW_L);
        }

        if (p < 0.0)
        {
            IncBurn(EThrusterPortDir::PITCH_D, EThrusterPortDir::PITCH_U);
        }
        else if (p > 0.0)
        {
            IncBurn(EThrusterPortDir::PITCH_U, EThrusterPortDir::PITCH_D);
        }
        else
        {
            DecBurn(EThrusterPortDir::PITCH_U, EThrusterPortDir::PITCH_D);
        }
    }
    else
    {
        for (int32 i = 0; i < NumThrusterDirections; ++i)
        {
            burn[i] -= 0.1f;

            if (burn[i] < 0.0f)
            {
                burn[i] = 0.0f;
            }
        }
    }

    for (int32 PortIndex = 0; PortIndex < ports.size(); ++PortIndex)
    {
        FThrusterPort* Port = ports[PortIndex];

        if (!Port)
        {
            continue;
        }

        Port->Burn = 0.0f;

        if (Port->Fire != 0)
        {
            int32 Flag = 1;

            for (int32 DirIndex = 0; DirIndex < NumThrusterDirections; ++DirIndex)
            {
                if ((Port->Fire & Flag) != 0)
                {
                    Port->Burn = FMath::Max(Port->Burn, burn[DirIndex]);
                }

                Flag <<= 1;
            }
        }
        else
        {
            const int32 DirIndex = ThrusterDirIndex(Port->Direction);

            if (DirIndex >= 0 && DirIndex < NumThrusterDirections)
            {
                Port->Burn = burn[DirIndex];
            }
        }

        Port->Burn *= thrust;

        if (Port->IntensityMultiplier > 0.0f)
        {
            Port->Burn *= Port->IntensityMultiplier;
        }

        Port->Burn = FMath::Clamp(Port->Burn, 0.0f, 1.0f);
    }

    ship->SetTransX(x * thrust);
    ship->SetTransY(y * thrust);
    ship->SetTransZ(z * thrust);
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

int Thruster::NumThrusters() const
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

float Thruster::GetIntensity(int index) const
{
    const FThrusterPort* Port = GetPort(index);

    if (!Port)
    {
        return 0.0f;
    }

    return Port->Burn;
}

// +----------------------------------------------------------------------+

float Thruster::GetVisualPower(int index) const
{
    return FMath::Clamp(GetIntensity(index), 0.0f, 1.0f);
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