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

    enum Type
    {
        SHIELD_FLASH = 1,
        HULL_FLASH = 2,
        BEAM_FLASH = 3,
        SHOT_BLAST = 4,
        HULL_BURST = 5,
        HULL_FIRE = 6,
        PLASMA_LEAK = 7,
        SMOKE_TRAIL = 8,
        SMALL_FIRE = 9,
        SMALL_EXPLOSION = 10,
        LARGE_EXPLOSION = 11,
        LARGE_BURST = 12,
        NUKE_EXPLOSION = 13,
        QUANTUM_FLASH = 14,
        HYPER_FLASH = 15
    };

    Explosion(int type,
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
    int               type;
    ParticleManager*  particles;

    float             scale;
    float             scale1;
    float             scale2;

    SimObject* source;
    FVector           mount_rel;
};
