/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipExplosion.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Ship explosion event definition used by ship death
    spiral sequences.
*/

#include "ShipExplosion.h"

// +--------------------------------------------------------------------+

ShipExplosion::ShipExplosion()
    : Type(0),
    Time(0.0f),
    Location(FVector::ZeroVector),
    bFinal(false)
{
}

// +--------------------------------------------------------------------+

int ShipExplosion::GetType() const
{
    return Type;
}

float ShipExplosion::GetTime() const
{
    return Time;
}

FVector ShipExplosion::GetLocation() const
{
    return Location;
}

bool ShipExplosion::IsFinal() const
{
    return bFinal;
}

// +--------------------------------------------------------------------+

void ShipExplosion::SetType(int InType)
{
    Type = InType;
}

void ShipExplosion::SetTime(float InTime)
{
    Time = InTime;
}

void ShipExplosion::SetLocation(const FVector& InLocation)
{
    Location = InLocation;
}

void ShipExplosion::SetFinal(bool bInFinal)
{
    bFinal = bInFinal;
}