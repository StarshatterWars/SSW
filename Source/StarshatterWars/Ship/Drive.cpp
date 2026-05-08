/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Drive.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Drive system class
*/

#include "Drive.h"

#include "Power.h"
#include "Ship.h"
#include "Bitmap.h"
#include "SimSystem.h"
#include "GameStructs_System.h"

#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

static int drive_value[] =
{
    1, 1, 1, 1, 1, 1, 1, 1
};

#define CLAMP(x, a, b) if ((x) < (a)) (x) = (a); else if ((x) > (b)) (x) = (b);

Bitmap* drive_flare_bitmap[8] =
{
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr
};

Bitmap* drive_trail_bitmap[8] =
{
    nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr
};

// +--------------------------------------------------------------------+

Drive::Drive(
    EDriveType InType,
    float MaxThrust,
    float MaxAug,
    bool bShow)
    : SimSystem(
        SYSTEM_CATEGORY::DRIVE,
        (int)InType,
        "Drive",
        drive_value[FMath::Clamp((int)InType, 0, 7)],
        MaxThrust * 2.0f,
        MaxThrust * 2.0f,
        MaxThrust * 2.0f),
    thrust(MaxThrust),
    augmenter(MaxAug),
    scale(0.0f),
    throttle(0.0f),
    augmenter_throttle(0.0f),
    intensity(0.0f),
    show_trail(bShow)
{
    power_flags = POWER_WATTS;

    name = "Drive";
    abrv = "DRV";

    emcon_power[0] = 0;
    emcon_power[1] = 50;
    emcon_power[2] = 100;
}

// +--------------------------------------------------------------------+

Drive::Drive(const Drive& d)
    : SimSystem(d),
    thrust(d.thrust),
    augmenter(d.augmenter),
    scale(d.scale),
    throttle(0.0f),
    augmenter_throttle(0.0f),
    intensity(0.0f),
    Ports(d.Ports),
    show_trail(d.show_trail)
{
    power_flags = POWER_WATTS;

    Mount(d);
}

// +--------------------------------------------------------------------+

Drive::~Drive()
{
    Ports.Empty();
}

// +--------------------------------------------------------------------+

void
Drive::Initialize()
{
    static int initialized = 0;

    if (initialized)
    {
        return;
    }

    initialized = 1;
}

// +--------------------------------------------------------------------+

void
Drive::StartFrame()
{
}

// +--------------------------------------------------------------------+

void
Drive::AddPort(const FDrivePort& Port)
{
    Ports.Add(Port);
}

// +--------------------------------------------------------------------+

void
Drive::CreatePort(const FVector& InLoc, float FlareScale)
{
    FDrivePort Port;

    Port.Location = InLoc;

    Port.FlareScale =
        (FlareScale > 0.0f)
        ? FlareScale
        : 1.0f;

    Port.TrailScale = Port.FlareScale;

    Port.bShowFlare = true;
    Port.bShowTrail = show_trail;

    Port.EngineColor = FLinearColor::White;

    Port.IntensityMultiplier = 1.0f;
    Port.AudioMultiplier = 1.0f;

    Port.PointName =
        FString::Printf(
            TEXT("Drive_%d"),
            Ports.Num());

    AddPort(Port);
}
// +--------------------------------------------------------------------+

void
Drive::Orient(const Physical* rep)
{
    SimSystem::Orient(rep);
}

// +--------------------------------------------------------------------+

static double drive_seconds = 0.0;

// +--------------------------------------------------------------------+

void
Drive::SetThrottle(double t, bool aug)
{
    const double Spool =
        1.2 * drive_seconds;

    const double ThrottleRequest =
        t / 100.0;

    if (throttle < ThrottleRequest)
    {
        if (ThrottleRequest - throttle < Spool)
        {
            throttle = (float)ThrottleRequest;
        }
        else
        {
            throttle += (float)Spool;
        }
    }
    else if (throttle > ThrottleRequest)
    {
        if (throttle - ThrottleRequest < Spool)
        {
            throttle = (float)ThrottleRequest;
        }
        else
        {
            throttle -= (float)Spool;
        }
    }

    if (throttle < 0.5f)
    {
        aug = false;
    }

    if (aug && augmenter_throttle < 1.0f)
    {
        augmenter_throttle += (float)Spool;

        if (augmenter_throttle > 1.0f)
        {
            augmenter_throttle = 1.0f;
        }
    }
    else if (!aug && augmenter_throttle > 0.0f)
    {
        augmenter_throttle -= (float)Spool;

        if (augmenter_throttle < 0.0f)
        {
            augmenter_throttle = 0.0f;
        }
    }
}

// +--------------------------------------------------------------------+

double
Drive::GetRequest(double seconds) const
{
    if (!power_on)
    {
        return 0.0;
    }

    const double TFactor =
        FMath::Max(
            throttle + (0.5 * augmenter_throttle),
            0.3);

    return
        TFactor *
        power_level *
        sink_rate *
        seconds;
}

// +--------------------------------------------------------------------+

bool
Drive::IsAugmenterOn() const
{
    return
        augmenter > 0.0f &&
        augmenter_throttle > 0.05f &&
        IsPowerOn() &&
        Status > SYSTEM_STATUS::CRITICAL;
}

// +--------------------------------------------------------------------+

int
Drive::NumEngines() const
{
    return Ports.Num();
}

// +--------------------------------------------------------------------+

float
Drive::Thrust(double seconds)
{
    drive_seconds = seconds;

    const float Denom =
        (capacity > 0.0f)
        ? capacity
        : 1.0f;

    float eff =
        (energy / Denom) *
        availability *
        100.0f;

    float output =
        throttle *
        thrust *
        eff;

    const bool aug_on =
        IsAugmenterOn();

    if (aug_on)
    {
        output +=
            augmenter *
            augmenter_throttle *
            eff;

        PowerSource* reac =
            ship
            ? ship->GetReactors()[source_index]
            : nullptr;

        if (reac)
        {
            reac->SetCapacity(
                reac->GetCapacity() -
                (0.1 * drive_seconds));
        }
    }

    energy = 0.0f;

    if (output < 0.0f ||
        GetPowerLevel() < 0.01f)
    {
        output = 0.0f;
    }

    const double fraction =
        (thrust != 0.0f)
        ? (output / thrust)
        : 0.0;

    if (fraction > 0.0)
    {
        intensity += (float)seconds;
    }
    else
    {
        intensity -= (float)seconds;
    }

    CLAMP(intensity, 0.0f, 1.0f);

    UE_LOG(LogTemp, Warning,
        TEXT("[Drive::Thrust] Ship=%p PowerOn=%d Throttle=%.2f AugThrottle=%.2f Intensity=%.2f MaxThrust=%.2f MaxAug=%.2f Request=%.2f Output=%.2f Seconds=%.4f"),
        ship,
        IsPowerOn() ? 1 : 0,
        throttle,
        augmenter_throttle,
        intensity,
        thrust,
        augmenter,
        GetRequest(seconds),
        output,
        seconds);

    return output;
}

// +--------------------------------------------------------------------+

EDriveType
Drive::GetDriveType() const
{
    return static_cast<EDriveType>(subtype);
}

// +--------------------------------------------------------------------+

float
Drive::GetThrottle() const
{
    return throttle;
}

// +--------------------------------------------------------------------+

float
Drive::GetAugmenterThrottle() const
{
    return augmenter_throttle;
}

// +--------------------------------------------------------------------+

float
Drive::GetIntensity() const
{
    return intensity;
}

// +--------------------------------------------------------------------+

float
Drive::GetVisualPower() const
{
    if (!IsPowerOn())
    {
        return 0.0f;
    }

    return FMath::Clamp(
        throttle + (0.5f * augmenter_throttle),
        0.0f,
        1.5f);
}

// +--------------------------------------------------------------------+

int
Drive::NumPorts() const
{
    return Ports.Num();
}

// +--------------------------------------------------------------------+

FVector
Drive::GetPortLocation(int Index) const
{
    if (Ports.IsValidIndex(Index))
    {
        return Ports[Index].Location;
    }

    return FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

float
Drive::GetPortScale(int Index) const
{
    if (Ports.IsValidIndex(Index))
    {
        return Ports[Index].FlareScale;
    }

    return 1.0f;
}