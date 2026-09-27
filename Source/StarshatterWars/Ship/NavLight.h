/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         NavLight.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Navigation Lights System class.

    Legacy runtime driver for Unreal Engine navigation lights.
    This class owns timing/state. Unreal actors only respond visually.
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "GameStructs_System.h"

#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Color.h"

class Physical;
class SimSystem;

class NavLight : public SimSystem
{
public:
    enum Constants
    {
        MAX_LIGHTS = 8
    };

    NavLight(double InPeriod, double InScale);
    NavLight(const NavLight& Rhs);
    virtual ~NavLight();

    static void Initialize();
    static void Close();

    virtual void ExecFrame(double Seconds);

    int  NumBeacons() const { return NumLights; }
    bool IsEnabled() const { return bEnabled; }

    virtual void Enable();
    virtual void Disable();

    virtual void AddBeacon(
        const FString& InName,
        const FVector& InLocation,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InIntensity,
        float InRadius,
        EShipNavLightMode InMode,
        float InBlinkInterval,
        float InPhaseOffset
    );

    virtual void SetPeriod(double InPeriod);
    virtual void SetScale(double InScale);
    virtual void SetOffset(double InOffset);

    virtual void Orient(const Physical* Rep);

    const char* GetBeaconName(int Index) const;
    FVector GetBeaconLocalLocation(int Index) const;
    FVector GetBeaconWorldLocation(int Index) const;
    FRotator GetBeaconLocalRotation(int Index) const;
    FLinearColor GetBeaconColor(int Index) const;
    float GetBeaconIntensity(int Index) const;
    float GetBeaconRadius(int Index) const;
    EShipNavLightMode GetBeaconMode(int Index) const;
    float GetBeaconBlinkInterval(int Index) const;
    float GetBeaconPhaseOffset(int Index) const;
    bool IsBeaconLit(int Index) const;

protected:
    bool IsValidBeaconIndex(int Index) const;

protected:
    double Period;
    double Scale;
    double OffsetSeconds;
    bool bEnabled;

    int NumLights;

    Text BeaconName[MAX_LIGHTS];

    FVector LocalLocation[MAX_LIGHTS];
    FVector WorldLocation[MAX_LIGHTS];
    FRotator LocalRotation[MAX_LIGHTS];

    FLinearColor Color[MAX_LIGHTS];
    float Intensity[MAX_LIGHTS];
    float Radius[MAX_LIGHTS];

    EShipNavLightMode Mode[MAX_LIGHTS];
    float BlinkInterval[MAX_LIGHTS];
    float PhaseOffset[MAX_LIGHTS];

    bool bLightOn[MAX_LIGHTS];
};