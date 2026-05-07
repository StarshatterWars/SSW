/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipExplosion.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Ship explosion event definition used by ship death
    spiral sequences.
*/

#pragma once

#include "Math/Vector.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class ShipExplosion
{
public:
    static const char* TYPENAME() { return "ShipExplosion"; }

    ShipExplosion();

    int     GetType() const;
    float   GetTime() const;
    FVector GetLocation() const;
    bool    IsFinal() const;

    static int GetMaxExplosions();

    void SetType(int InType);
    void SetTime(float InTime);
    void SetLocation(const FVector& InLocation);
    void SetFinal(bool bInFinal);

    void CopyFrom(const ShipExplosion& Src);
    
    static const int MaxExplosions = 10;

protected:
    int     Type;
    float   Time;
    FVector Location;
    bool    bFinal;
    
};
