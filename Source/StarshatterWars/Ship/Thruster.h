/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Thruster.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Conventional Thruster (system) class
*/

#pragma once

#include "Types.h"
#include "SimSystem.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+
// Forward Declarations
// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class Thruster : public SimSystem
{
public:
    static const char* TYPENAME() { return "Thruster"; }

    static constexpr int32 NumThrusterDirections = 12;

    Thruster(int dtype, double thrust, float flare_scale = 0);
    Thruster(const Thruster& rhs);
    virtual ~Thruster();

    virtual void   ExecFrame(double seconds);
    virtual void   ExecTrans(double x, double y, double z);
    virtual void   SetShip(Ship* s);

    virtual double TransXLimit();
    virtual double TransYLimit();
    virtual double TransZLimit();

    virtual void AddPort(
        EThrusterPortDir Dir,
        const FVector& Loc,
        DWORD Fire,
        float FlareScale = 0.0f);

    virtual void SetPortData(
        int index,
        const FThrusterPort& InPort);

    int GetNumThrusters() const;

    const FThrusterPort* GetPort(int index) const;

    FVector GetPortLocation(int index) const;
    FRotator GetPortRotation(int index) const;

    float GetPortScale(int index) const;
    float GetThrusterBurn(int index) const;
    float GetVisualPower(int index) const;

    DWORD GetPortFireFlags(int index) const;

    virtual double GetRequest(double seconds) const;

    void SetThrust(double InThrust)
    {
        thrust = (float)InThrust;
    }

    void SetThrusterScale(float InScale)
    {
        scale = InScale;
    }

    void SetHullFactor(float InHullFactor)
    {
        hull_factor = InHullFactor;
    }

protected:
    void IncBurn(
        EThrusterPortDir Inc,
        EThrusterPortDir Dec);

    void DecBurn(
        EThrusterPortDir A,
        EThrusterPortDir B);

protected:
    Ship* ship = nullptr;

    float thrust = 0.0f;
    float scale = 1.0f;

    float burn[NumThrusterDirections] = { 0 };

    float avail_x = 0.0f;
    float avail_y = 0.0f;
    float avail_z = 0.0f;

    float trans_x = 0.0f;
    float trans_y = 0.0f;
    float trans_z = 0.0f;

    float roll_rate = 0.0f;
    float pitch_rate = 0.0f;
    float yaw_rate = 0.0f;

    float roll_drag = 0.0f;
    float pitch_drag = 0.0f;
    float yaw_drag = 0.0f;

    List<FThrusterPort> ports;
};