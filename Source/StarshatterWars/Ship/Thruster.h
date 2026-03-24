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

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+
// Forward Declarations
// +--------------------------------------------------------------------+

class Ship;
class Graphic;
class Physical;

// +--------------------------------------------------------------------+

struct ThrusterPort
{
    static const char* TYPENAME() { return "ThrusterPort"; }

    ThrusterPort(int t, const FVector& l, DWORD f, float s);
    ~ThrusterPort();

    int      type = 0;
    DWORD    fire = 0;
    float    burn = 0.0f;
    float    scale = 0.0f;

    FVector  loc = FVector::ZeroVector;

    Graphic* flare = nullptr;
    Graphic* trail = nullptr;
};

// +--------------------------------------------------------------------+

class Thruster : public SimSystem
{
public:
    static const char* TYPENAME() { return "Thruster"; }

    enum Constants
    {
        LEFT, RIGHT, FORE, AFT, TOP, BOTTOM,
        YAW_L, YAW_R, PITCH_D, PITCH_U, ROLL_L, ROLL_R
    };

    Thruster(int dtype, double thrust, float flare_scale = 0);
    Thruster(const Thruster& rhs);
    virtual ~Thruster();

    static void    Initialize();
    static void    Close();

    virtual void   ExecFrame(double seconds);
    virtual void   ExecTrans(double x, double y, double z);
    virtual void   SetShip(Ship* s);

    virtual double TransXLimit();
    virtual double TransYLimit();
    virtual double TransZLimit();

    virtual void   AddPort(int type, const FVector& loc, DWORD fire, float flare_scale = 0);
    virtual void   CreatePort(int type, const FVector& loc, DWORD fire, float flare_scale);

    int            NumThrusters() const;
    Graphic* Flare(int engine) const;
    Graphic* Trail(int engine) const;

    virtual void   Orient(const Physical* rep);
    virtual double GetRequest(double seconds) const;

protected:
    void           IncBurn(int inc, int dec);
    void           DecBurn(int a, int b);

    Ship* ship = nullptr;

    float          thrust = 0.0f;
    float          scale = 0.0f;
    float          burn[12] = { 0 };

    float          avail_x = 0.0f;
    float          avail_y = 0.0f;
    float          avail_z = 0.0f;

    float          trans_x = 0.0f;
    float          trans_y = 0.0f;
    float          trans_z = 0.0f;

    float          roll_rate = 0.0f;
    float          pitch_rate = 0.0f;
    float          yaw_rate = 0.0f;

    float          roll_drag = 0.0f;
    float          pitch_drag = 0.0f;
    float          yaw_drag = 0.0f;

    List<ThrusterPort> ports;
};
