/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         NavLight.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Navigation Light System class.

    Legacy runtime driver for Unreal Engine navigation lights.
*/

#include "NavLight.h"

#include "Game.h"
#include "Physical.h"
#include "Camera.h"

#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include <cstring>

NavLight::NavLight(double InPeriod, double InScale)
    : SimSystem(SYSTEM_CATEGORY::COMPUTER, 32, "Navigation Lights", 1, 0)
    , Period(InPeriod)
    , Scale(InScale)
    , OffsetSeconds(0.0)
    , bEnabled(true)
    , NumLights(0)
{
    name = "Navigation Lights";
    abrv = "NavLights";

    for (int Index = 0; Index < MAX_LIGHTS; ++Index)
    {
        BeaconName[Index] = "";
        LocalLocation[Index] = FVector::ZeroVector;
        WorldLocation[Index] = FVector::ZeroVector;
        LocalRotation[Index] = FRotator::ZeroRotator;

        Color[Index] = FLinearColor::White;
        Intensity[Index] = 1200.0f;
        Radius[Index] = 200.0f;

        Mode[Index] = EShipNavLightMode::Blink;
        BlinkInterval[Index] = 0.5f;
        PhaseOffset[Index] = 0.0f;

        bLightOn[Index] = false;
    }
}

NavLight::NavLight(const NavLight& Rhs)
    : SimSystem(Rhs)
    , Period(Rhs.Period)
    , Scale(Rhs.Scale)
    , OffsetSeconds((double)FMath::FRand())
    , bEnabled(Rhs.bEnabled)
    , NumLights(Rhs.NumLights)
{
    Mount(Rhs);
    SetAbbreviation(Rhs.Abbreviation());

    for (int Index = 0; Index < MAX_LIGHTS; ++Index)
    {
        BeaconName[Index] = Rhs.BeaconName[Index];
        LocalLocation[Index] = Rhs.LocalLocation[Index];
        WorldLocation[Index] = Rhs.WorldLocation[Index];
        LocalRotation[Index] = Rhs.LocalRotation[Index];

        Color[Index] = Rhs.Color[Index];
        Intensity[Index] = Rhs.Intensity[Index];
        Radius[Index] = Rhs.Radius[Index];

        Mode[Index] = Rhs.Mode[Index];
        BlinkInterval[Index] = Rhs.BlinkInterval[Index];
        PhaseOffset[Index] = Rhs.PhaseOffset[Index];

        bLightOn[Index] = false;
    }
}

NavLight::~NavLight()
{
}

void NavLight::Initialize()
{
}

void NavLight::Close()
{
}

void NavLight::ExecFrame(double Seconds)
{
    const double GameSeconds = Game::GetGameTime() / 1000.0;

    if (bEnabled && power_on)
    {
        for (int Index = 0; Index < NumLights; ++Index)
        {
            if (Mode[Index] == EShipNavLightMode::Steady)
            {
                bLightOn[Index] = true;
            }
            else
            {
                const float Interval = FMath::Max(0.05f, BlinkInterval[Index]);
                const double T = GameSeconds + OffsetSeconds + PhaseOffset[Index];
                const double LocalTime = FMath::Fmod(T, (double)Interval);

                bLightOn[Index] = LocalTime < ((double)Interval * 0.5);
            }
        }
    }
    else
    {
        for (int Index = 0; Index < NumLights; ++Index)
        {
            bLightOn[Index] = false;
        }
    }

    SimSystem::ExecFrame(Seconds);
}

void NavLight::Enable()
{
    bEnabled = true;
}

void NavLight::Disable()
{
    bEnabled = false;
}

void NavLight::AddBeacon(
    const FString& InName,
    const FVector& InLocation,
    const FRotator& InRotation,
    const FLinearColor& InColor,
    float InIntensity,
    float InRadius,
    EShipNavLightMode InMode,
    float InBlinkInterval,
    float InPhaseOffset)
{
    if (NumLights >= MAX_LIGHTS)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[NavLight] AddBeacon failed: MAX_LIGHTS reached"));
        return;
    }

    const int Index = NumLights++;

    BeaconName[Index] = TCHAR_TO_ANSI(*InName);
    LocalLocation[Index] = InLocation;
    WorldLocation[Index] = InLocation;
    LocalRotation[Index] = InRotation;

    Color[Index] = InColor;
    Intensity[Index] = InIntensity;
    Radius[Index] = InRadius;

    Mode[Index] = InMode;
    BlinkInterval[Index] = InBlinkInterval > 0.0f ? InBlinkInterval : (float)Period;
    PhaseOffset[Index] = InPhaseOffset;

    bLightOn[Index] = false;
}

void NavLight::SetPeriod(double InPeriod)
{
    Period = InPeriod;
}

void NavLight::SetScale(double InScale)
{
    Scale = InScale;
}

void NavLight::SetOffset(double InOffset)
{
    OffsetSeconds = InOffset;
}

void NavLight::Orient(const Physical* Rep)
{
    SimSystem::Orient(Rep);

    if (!Rep)
    {
        for (int Index = 0; Index < NumLights; ++Index)
        {
            WorldLocation[Index] = LocalLocation[Index];
        }

        return;
    }

    const Camera& RepCam = Rep->GetCam();
    const FVector ShipLoc = Rep->GetLocation();

    const FMatrix Basis(
        FPlane(RepCam.vrt(), 0.0f),
        FPlane(RepCam.vup(), 0.0f),
        FPlane(RepCam.vpn(), 0.0f),
        FPlane(0.0f, 0.0f, 0.0f, 1.0f)
    );

    for (int Index = 0; Index < NumLights; ++Index)
    {
        WorldLocation[Index] =
            Basis.TransformVector(LocalLocation[Index] * Scale) + ShipLoc;
    }
}

bool NavLight::IsValidBeaconIndex(int Index) const
{
    return Index >= 0 && Index < NumLights;
}

const char* NavLight::GetBeaconName(int Index) const
{
    return IsValidBeaconIndex(Index) ? BeaconName[Index].data() : "";
}

FVector NavLight::GetBeaconLocalLocation(int Index) const
{
    return IsValidBeaconIndex(Index) ? LocalLocation[Index] : FVector::ZeroVector;
}

FVector NavLight::GetBeaconWorldLocation(int Index) const
{
    return IsValidBeaconIndex(Index) ? WorldLocation[Index] : FVector::ZeroVector;
}

FRotator NavLight::GetBeaconLocalRotation(int Index) const
{
    return IsValidBeaconIndex(Index) ? LocalRotation[Index] : FRotator::ZeroRotator;
}

FLinearColor NavLight::GetBeaconColor(int Index) const
{
    return IsValidBeaconIndex(Index) ? Color[Index] : FLinearColor::White;
}

float NavLight::GetBeaconIntensity(int Index) const
{
    return IsValidBeaconIndex(Index) ? Intensity[Index] : 0.0f;
}

float NavLight::GetBeaconRadius(int Index) const
{
    return IsValidBeaconIndex(Index) ? Radius[Index] : 0.0f;
}

EShipNavLightMode NavLight::GetBeaconMode(int Index) const
{
    return IsValidBeaconIndex(Index) ? Mode[Index] : EShipNavLightMode::Steady;
}

float NavLight::GetBeaconBlinkInterval(int Index) const
{
    return IsValidBeaconIndex(Index) ? BlinkInterval[Index] : 0.0f;
}

float NavLight::GetBeaconPhaseOffset(int Index) const
{
    return IsValidBeaconIndex(Index) ? PhaseOffset[Index] : 0.0f;
}

bool NavLight::IsBeaconLit(int Index) const
{
    return IsValidBeaconIndex(Index) ? bLightOn[Index] : false;
}