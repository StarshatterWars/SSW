/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Explosion.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Explosion Sprite class
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "Sound.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class Solid;
class ParticleManager;
class SimSystem;
class SimRegion;

// +--------------------------------------------------------------------+

class Explosion : public SimObject,
    public SimObserver
{
public:
    static const char* TYPENAME() { return "Explosion"; }

    Explosion(EExplosionType type,
        const FVector& pos,
        const FVector& vel,
        float exp_scale,
        float part_scale,
        SimRegion* rgn = 0,
        SimObject* source = 0);

    virtual ~Explosion();

    static void       Initialize();
    static void       Close();

    virtual void      ExecFrame(double seconds);

    ParticleManager*  GetParticles() { return particles; }

    virtual void      Activate(SimScene& scene);
    virtual void      Deactivate(SimScene& scene);

    // SimObserver interface:
    virtual bool         Update(SimObject* obj);
    virtual const char* GetObserverName() const;

protected:
    EExplosionType               type;
    ParticleManager*             particles;

    float             scale;
    float             scale1;
    float             scale2;

    SimObject* source;
    FVector           mount_rel;
};
