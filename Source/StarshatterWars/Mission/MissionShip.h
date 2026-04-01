/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionShip.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionShip

    Runtime mission ship state container.

    Represents a ship instance within a mission element.
    Stores identity, spatial data, and per-ship gameplay state.

    ARCHITECTURE ROLE
    =================
    FShipDesign   -> static ship definition
    MissionShip   -> runtime instance state (THIS CLASS)

    NOTES
    =====
    - This is NOT a design definition
    - This is NOT Unreal-facing
    - Used only during mission runtime and parsing
    - Ammo and fuel arrays are legacy fixed-size
*/

#pragma once

#include "Text.h"
#include "Math/Vector.h"

class Skin;

class MissionShip
{
public:
    static const char* TYPENAME() { return "MissionShip"; }

public:
    MissionShip();
    ~MissionShip() {}

    // Identity
    const Text& GetName()   const;
    const Text& GetRegNum() const;
    const Text& GetRegion() const;

    // Visual
    const Skin* GetSkin() const;

    // Spatial
    const FVector& GetLocation() const;
    const FVector& GetVelocity() const;

    // Gameplay State
    int     GetRespawns()  const;
    double  GetHeading()   const;
    double  GetIntegrity() const;

    int     GetDecoys() const;
    int     GetProbes() const;

    const int* GetAmmo() const;
    const int* GetFuel() const;

    // Setters
    void SetName(const char* n);
    void SetRegNum(const char* n);
    void SetRegion(const char* n);
    void SetSkin(const Skin* s);

    void SetLocation(const FVector& p);
    void SetVelocity(const FVector& p);

    void SetRespawns(int r);
    void SetHeading(double h);
    void SetIntegrity(double n);

    void SetDecoys(int d);
    void SetProbes(int p);

    void SetAmmo(const int* a);
    void SetFuel(const int* f);

private:
    static const int MAX_AMMO = 16;
    static const int MAX_FUEL = 4;

    Text name;
    Text regnum;
    Text region;

    const Skin* skin;

    FVector loc;
    FVector velocity;

    int    respawns;
    double heading;
    double integrity;

    int decoys;
    int probes;

    int ammo[MAX_AMMO];
    int fuel[MAX_FUEL];
};