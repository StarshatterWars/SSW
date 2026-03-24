/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         LandingGear.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    LandingGear System class

    Original Author and Studio:
    John DiCamillo / Destroyer Studios
*/

#include "LandingGear.h"

#include "Ship.h"
#include "Sim.h"
#include "AudioConfig.h"

#include "DataLoader.h"
#include "Physical.h"
#include "SimModel.h"
#include "SimScene.h"
#include "Sound.h"
#include "Game.h"

#include "Math/Vector.h"
#include "Logging/LogMacros.h"

static USound* gear_transit_sound = nullptr;

// +----------------------------------------------------------------------+

LandingGear::LandingGear()
    : SimSystem(SYSTEM_CATEGORY::MISC_SYSTEM, 0, "Landing Gear", 1, 1, 1, 1),
    state(GEAR_UP),
    transit(0),
    clearance(0),
    ngear(0)
{
    name = "Landing Gear";
    abrv = "Gear";

    for (int i = 0; i < MAX_GEAR; i++) {
        models[i] = nullptr;
        gear[i] = nullptr;
        start[i] = FVector::ZeroVector;
        end[i] = FVector::ZeroVector;
    }
}

// +----------------------------------------------------------------------+

LandingGear::LandingGear(const LandingGear& g)
    : SimSystem(g),
    state(GEAR_UP),
    transit(0),
    clearance(0),
    ngear(g.ngear)
{
    Mount(g);
    SetAbbreviation(g.Abbreviation());

    int i = 0;

    for (i = 0; i < ngear; i++) {
        models[i] = nullptr;

        gear[i] = new Solid; // removed (__FILE__, __LINE__)
        start[i] = g.start[i];
        end[i] = g.end[i];

        if (gear[i])
            gear[i]->UseModel(g.models[i]);

        // Track lowest Y of the gear stop points (end positions):
        if (i == 0 || clearance > end[i].Y)
            clearance = end[i].Y;
    }

    while (i < MAX_GEAR) {
        models[i] = nullptr;
        gear[i] = nullptr;
        start[i] = FVector::ZeroVector;
        end[i] = FVector::ZeroVector;
        i++;
    }

    // mount_rel is inherited from SimSystem/System base; keep semantics:
    clearance += mount_rel.Y;
}

// +--------------------------------------------------------------------+

LandingGear::~LandingGear()
{
    for (int i = 0; i < MAX_GEAR; i++) {
        delete models[i];
        models[i] = nullptr;

        if (gear[i]) {
            Solid* g = gear[i];

            if (g->GetScene())
                g->GetScene()->DelGraphic(g);

            delete g;
            gear[i] = nullptr;
        }
    }
}

// +--------------------------------------------------------------------+

void LandingGear::Initialize()
{
    if (!gear_transit_sound) {
        DataLoader* loader = DataLoader::GetLoader();

        // Unreal conversion note:
        // DataLoader::SetDataPath expects FString in your UE port.
        // Keep call site valid by using TEXT("") literal (FString-compatible).
        loader->SetDataPath("Sounds/");
        loader->LoadSound("GearTransit.wav", gear_transit_sound);
    }
}

// +--------------------------------------------------------------------+

void LandingGear::Close()
{
    delete gear_transit_sound;
    gear_transit_sound = nullptr;
}

// +--------------------------------------------------------------------+

int LandingGear::AddGear(SimModel* m, const FVector& s, const FVector& e)
{
    if (ngear < MAX_GEAR) {
        models[ngear] = m;
        start[ngear] = s;
        end[ngear] = e;

        ngear++;
    }

    return ngear;
}

// +--------------------------------------------------------------------+

void LandingGear::SetState(GEAR_STATE s)
{
    if (state == s)
        return;

    state = s;

    if (ship && ship == Sim::GetSim()->GetPlayerShip()) {
        if (state == GEAR_LOWER || state == GEAR_RAISE) {
            if (gear_transit_sound) {
                USound* sound = gear_transit_sound->Duplicate();
                if (sound) {
                    sound->SetVolume(AudioConfig::EfxVolume());
                    sound->Play();
                }
            }
        }
    }
}

// +--------------------------------------------------------------------+

void LandingGear::ExecFrame(double seconds)
{
    SimSystem::ExecFrame(seconds);

    switch (state) {
    case GEAR_UP:
        transit = 0;
        break;

    case GEAR_DOWN:
        transit = 1;
        break;

    case GEAR_LOWER:
        if (transit < 1) {
            transit += seconds;

            SimScene* s = nullptr;
            if (ship && ship->Rep())
                s = ship->Rep()->GetScene();

            if (s) {
                for (int i = 0; i < ngear; i++) {
                    if (gear[i] && !gear[i]->GetScene()) {
                        s->AddGraphic(gear[i]);
                    }
                }
            }
        }
        else {
            transit = 1;
            state = GEAR_DOWN;
        }
        break;

    case GEAR_RAISE:
        if (transit > 0) {
            transit -= seconds;
        }
        else {
            transit = 0;
            state = GEAR_UP;

            for (int i = 0; i < ngear; i++) {
                if (gear[i]) {
                    SimScene* s = gear[i]->GetScene();
                    if (s)
                        s->DelGraphic(gear[i]);
                }
            }
        }
        break;

    default:
        break;
    }
}

// +--------------------------------------------------------------------+

void
LandingGear::Orient(const Physical* rep)
{
    SimSystem::Orient(rep);

    if (!rep)
        return;

    const FVector ShipLoc = rep->Location();

    // Build a UE transform from the ship camera basis (same convention as FlightDeck):
    FVector XAxis = rep->Cam().vrt(); // right
    FVector YAxis = rep->Cam().vup(); // up
    FVector ZAxis = rep->Cam().vpn(); // forward

    XAxis = XAxis.GetSafeNormal();
    YAxis = YAxis.GetSafeNormal();
    ZAxis = ZAxis.GetSafeNormal();

    // Defensive orthonormalization:
    ZAxis = (ZAxis - (ZAxis | XAxis) * XAxis).GetSafeNormal();
    YAxis = (XAxis ^ ZAxis).GetSafeNormal();

    const FMatrix ShipM(
        FPlane(XAxis.X, XAxis.Y, XAxis.Z, 0.0f),
        FPlane(YAxis.X, YAxis.Y, YAxis.Z, 0.0f),
        FPlane(ZAxis.X, ZAxis.Y, ZAxis.Z, 0.0f),
        FPlane(ShipLoc.X, ShipLoc.Y, ShipLoc.Z, 1.0f)
    );

    const FTransform ShipXform(ShipM);

    // If your gear solids need the same ship orientation each frame:
    const FMatrix GearOrientationM = ShipM;

    for (int i = 0; i < ngear; i++)
    {
        FVector Gloc = FVector::ZeroVector;

        if (transit < 1.0)
            Gloc = start[i] + (end[i] - start[i]) * (float)transit;
        else
            Gloc = end[i];

        // Transform gear-relative location into world space:
        const FVector GearWorldLoc = ShipXform.TransformPosition(Gloc);

        if (gear[i])
        {
            gear[i]->MoveTo(GearWorldLoc);
            gear[i]->SetOrientation(GearOrientationM);
        }
    }
}

// +--------------------------------------------------------------------+

Solid* LandingGear::GetGear(int index)
{
    if (index >= 0 && index < ngear) {
        return gear[index];
    }

    return nullptr;
}

// +--------------------------------------------------------------------+

FVector LandingGear::GetGearStop(int index)
{
    if (index >= 0 && index < ngear) {
        Solid* g = gear[index];
        if (g)
            return g->Location();
    }

    return FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

double LandingGear::GetTouchDown()
{
    double down = 0;

    if (ship) {
        down = ship->Location().Y;

        if (state != GEAR_UP) {
            for (int i = 0; i < ngear; i++) {
                if (gear[i]) {
                    const FVector stop = gear[i]->Location();

                    if (stop.Y < down)
                        down = stop.Y;
                }
            }
        }
    }

    return down;
}
