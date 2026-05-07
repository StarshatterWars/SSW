/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipDebris.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Ship debris definition used by ship death
    spiral sequences.
*/

#include "ShipDebris.h"
#include "SimModel.h"

// +--------------------------------------------------------------------+

ShipDebris::ShipDebris()
    : Model(nullptr),
    Count(0),
    Life(0),
    Location(FVector::ZeroVector),
    Mass(0.0f),
    Speed(0.0f),
    Drag(0.0f),
    FireType(0)
{
    for (int i = 0; i < MAX_FIRE_LOCATIONS; i++)
    {
        FireLocation[i] = FVector::ZeroVector;
    }
}

// +--------------------------------------------------------------------+

SimModel*
ShipDebris::GetModel() const
{
    return Model;
}

int
ShipDebris::GetCount() const
{
    return Count;
}

int
ShipDebris::GetLife() const
{
    return Life;
}

FVector
ShipDebris::GetLocation() const
{
    return Location;
}

float
ShipDebris::GetMass() const
{
    return Mass;
}

float
ShipDebris::GetSpeed() const
{
    return Speed;
}

float
ShipDebris::GetDrag() const
{
    return Drag;
}

int
ShipDebris::GetFireType() const
{
    return FireType;
}

FVector
ShipDebris::GetFireLocation(int Index) const
{
    if (Index >= 0 && Index < MAX_FIRE_LOCATIONS)
    {
        return FireLocation[Index];
    }

    return FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

void
ShipDebris::SetModel(SimModel* InModel)
{
    Model = InModel;
}

void
ShipDebris::SetCount(int InCount)
{
    Count = InCount;
}

void
ShipDebris::SetLife(int InLife)
{
    Life = InLife;
}

void
ShipDebris::SetLocation(const FVector& InLocation)
{
    Location = InLocation;
}

void
ShipDebris::SetMass(float InMass)
{
    Mass = InMass;
}

void
ShipDebris::SetSpeed(float InSpeed)
{
    Speed = InSpeed;
}

void
ShipDebris::SetDrag(float InDrag)
{
    Drag = InDrag;
}

void
ShipDebris::SetFireType(int InFireType)
{
    FireType = InFireType;
}

void
ShipDebris::SetFireLocation(int Index, const FVector& InLocation)
{
    if (Index >= 0 && Index < MAX_FIRE_LOCATIONS)
    {
        FireLocation[Index] = InLocation;
    }
}