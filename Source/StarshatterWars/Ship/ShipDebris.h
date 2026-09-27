/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipDebris.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Ship debris definition used by ship death
    spiral sequences.
*/

#pragma once

#include "Math/Vector.h"
#include "Types.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class SimModel;
class ShipDesign;

// +--------------------------------------------------------------------+

class ShipDebris
{
public:
    static const char* TYPENAME() { return "ShipDebris"; }

    enum CONSTANTS { MAX_FIRE_LOCATIONS = 5 };

    ShipDebris();

    SimModel* GetModel() const;
    int       GetCount() const;
    int       GetLife() const;
    FVector   GetLocation() const;
    float     GetMass() const;
    float     GetSpeed() const;
    float     GetDrag() const;
    int       GetFireType() const;
    FVector   GetFireLocation(int Index) const;

    void SetModel(SimModel* InModel);
    void SetCount(int InCount);
    void SetLife(int InLife);
    void SetLocation(const FVector& InLocation);
    void SetMass(float InMass);
    void SetSpeed(float InSpeed);
    void SetDrag(float InDrag);
    void SetFireType(int InFireType);
    void SetFireLocation(int Index, const FVector& InLocation);

protected:
    SimModel* Model;
    int       Count;
    int       Life;
    FVector   Location;
    float     Mass;
    float     Speed;
    float     Drag;
    int       FireType;
    FVector   FireLocation[MAX_FIRE_LOCATIONS];
};
