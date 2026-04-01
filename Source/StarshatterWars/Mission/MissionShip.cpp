/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionShip.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionShip implementation

    Handles runtime ship state storage for missions.
*/

#include "MissionShip.h"

// +--------------------------------------------------------------------+

MissionShip::MissionShip()
    : loc(-1e9f, -1e9f, -1e9f),
    velocity(-1e9f, -1e9f, -1e9f),
    respawns(0),
    heading(0),
    integrity(100),
    decoys(-10),
    probes(-10),
    skin(nullptr)
{
    for (int i = 0; i < MAX_AMMO; i++)
        ammo[i] = -10;

    for (int i = 0; i < MAX_FUEL; i++)
        fuel[i] = -10;
}

// +--------------------------------------------------------------------+

const Text& MissionShip::GetName() const { return name; }
const Text& MissionShip::GetRegNum() const { return regnum; }
const Text& MissionShip::GetRegion() const { return region; }

const Skin* MissionShip::GetSkin() const { return skin; }

const FVector& MissionShip::GetLocation() const { return loc; }
const FVector& MissionShip::GetVelocity() const { return velocity; }

int MissionShip::GetRespawns() const { return respawns; }
double MissionShip::GetHeading() const { return heading; }
double MissionShip::GetIntegrity() const { return integrity; }

int MissionShip::GetDecoys() const { return decoys; }
int MissionShip::GetProbes() const { return probes; }

const int* MissionShip::GetAmmo() const { return ammo; }
const int* MissionShip::GetFuel() const { return fuel; }

// +--------------------------------------------------------------------+

void MissionShip::SetName(const char* n) { name = n; }
void MissionShip::SetRegNum(const char* n) { regnum = n; }
void MissionShip::SetRegion(const char* n) { region = n; }
void MissionShip::SetSkin(const Skin* s) { skin = s; }

void MissionShip::SetLocation(const FVector& p) { loc = p; }
void MissionShip::SetVelocity(const FVector& p) { velocity = p; }

void MissionShip::SetRespawns(int r) { respawns = r; }
void MissionShip::SetHeading(double h) { heading = h; }
void MissionShip::SetIntegrity(double n) { integrity = n; }

void MissionShip::SetDecoys(int d) { decoys = d; }
void MissionShip::SetProbes(int p) { probes = p; }

// +--------------------------------------------------------------------+

void MissionShip::SetAmmo(const int* a)
{
    if (!a)
        return;

    for (int i = 0; i < MAX_AMMO; i++)
    {
        ammo[i] = a[i];
    }
}

void MissionShip::SetFuel(const int* f)
{
    if (!f)
        return;

    for (int i = 0; i < MAX_FUEL; i++)
    {
        fuel[i] = f[i];
    }
}