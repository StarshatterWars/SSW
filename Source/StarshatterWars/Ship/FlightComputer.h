/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         FlightComputer.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Flight Computer systems class
*/

#pragma once

#include "Types.h"
#include "Computer.h"

#include "Math/Vector.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class FlightComputer : public Computer
{
public:
    enum CompType
    {
        AVIONICS = 1,
        FLIGHT,
        TACTICAL
    };

    FlightComputer(
        EComputerType comp_type,
        const char* comp_name);

    FlightComputer(
        const Computer& rhs);

    virtual ~FlightComputer();

    //-------------------------------------------------------------
    // Main update
    //-------------------------------------------------------------
    virtual void ExecSubFrame();

    //-------------------------------------------------------------
    // Accessors
    //-------------------------------------------------------------
    int Mode() const
    {
        return mode;
    }

    double Throttle() const
    {
        return throttle;
    }

    double VelocityLimit() const
    {
        return vlimit;
    }

    double TransXLimit() const
    {
        return trans_x_limit;
    }

    double TransYLimit() const
    {
        return trans_y_limit;
    }

    double TransZLimit() const
    {
        return trans_z_limit;
    }

    bool IsHalting() const
    {
        return halt != 0;
    }

    //-------------------------------------------------------------
    // Mutators
    //-------------------------------------------------------------
    void SetMode(int m)
    {
        mode = m;
    }

    void SetThrottle(double t)
    {
        throttle = (float)t;
    }

    void SetVelocityLimit(double v)
    {
        vlimit = (float)v;
    }

    void SetTransLimit(
        double x,
        double y,
        double z);

    void FullStop()
    {
        halt = true;
    }

    void ClearHalt()
    {
        halt = false;
    }

protected:
    //-------------------------------------------------------------
    // Internal update stages
    //-------------------------------------------------------------
    virtual void ExecTrans();
    virtual void ExecThrottle();

    //-------------------------------------------------------------
    // Local-space helpers
    //-------------------------------------------------------------
    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;

    double GetForwardVelocity() const;
    double GetSideVelocity() const;
    double GetVerticalVelocity() const;

protected:
    //-------------------------------------------------------------
    // Flight state
    //-------------------------------------------------------------
    int     mode;
    int     halt;

    //-------------------------------------------------------------
    // Cached throttle state
    //-------------------------------------------------------------
    float   throttle;

    //-------------------------------------------------------------
    // Velocity limits
    //-------------------------------------------------------------
    float   vlimit;

    //-------------------------------------------------------------
    // Translational correction limits
    //-------------------------------------------------------------
    float   trans_x_limit;
    float   trans_y_limit;
    float   trans_z_limit;
};