/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Drive.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Drive system class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "GameStructs_System.h"

#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class Bolt;
class DriveSprite;
class Light;
class Ship;
class Physical;

// +--------------------------------------------------------------------+

class Drive : public SimSystem
{
public:

    enum Constants
    {
        MAX_ENGINES = 16
    };

    Drive(
        EDriveType InType,
        float max_thrust,
        float max_aug,
        bool show_trail = true);

    Drive(const Drive& rhs);

    virtual ~Drive();

    static void Initialize();
    static void StartFrame();

    float Thrust(double seconds);

    float MaxThrust() const
    {
        return thrust;
    }

    float MaxAugmenter() const
    {
        return augmenter;
    }

    int NumEngines() const;

    bool IsAugmenterOn() const;

    virtual void AddPort(const FDrivePort& Port);

    virtual void CreatePort(
        const FVector& loc,
        float flare_scale);

    virtual void Orient(
        const Physical* rep);

    void SetThrottle(
        double InThrottle,
        bool aug = false);

    virtual double GetRequest(
        double seconds) const;

    // ------------------------------------------------------------
    // Runtime accessors for Unreal engine FX/audio integration
    // ------------------------------------------------------------

    EDriveType GetDriveType() const;

    float GetThrottle() const;
    float GetAugmenterThrottle() const;
    float GetIntensity() const;
    float GetVisualPower() const;

    int NumPorts() const;

    FVector GetPortLocation(int Index) const;
    float GetPortScale(int Index) const;

protected:

    float thrust;
    float augmenter;
    float scale;

    float throttle;
    float augmenter_throttle;
    float intensity;

    TArray<FDrivePort> Ports;

    bool show_trail;
};