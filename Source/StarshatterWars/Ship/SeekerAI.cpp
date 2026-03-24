/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         SeekerAI.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    OVERVIEW
    ========
    Seeker Missile (low-level) Artificial Intelligence class
*/

#include "SeekerAI.h"

#include "Ship.h"
#include "SimShot.h"
#include "SimSystem.h"
#include "WeaponDesign.h"

#include "Game.h"

#include "CoreMinimal.h"              // UE_LOG
#include "Math/UnrealMathUtility.h"   // FMath
#include "Math/Vector.h"              // FVector

// NOTE:
// - Removed MemDebug.h (not supported in UE builds).
// - Replaced legacy rand() usage with UE FMath equivalents.
// - Converted Point/Vec3 math to FVector.
// - This remains a plain C++ class (not a UObject) and keeps Starshatter core types intact.

// +----------------------------------------------------------------------+

SeekerAI::SeekerAI(SimObject* s)
    : SteerAI(s),
    orig_target(nullptr),
    shot((SimShot*)s),
    pursuit(1),
    delay(0),
    overshot(false)
{
    ai_type = SEEKER;

    seek_gain = 25;
    seek_damp = 0.55;
}

// +--------------------------------------------------------------------+

SeekerAI::~SeekerAI()
{
    if (shot) {
        if (shot->Owner())
            ((Ship*)shot->Owner())->SetMissileEta(shot->Identity(), 0);
    }
}

// +--------------------------------------------------------------------+

void
SeekerAI::ExecFrame(double seconds)
{
    (void)seconds;

    // setup:
    FindObjective();

    // adaptive behavior:
    Navigator();
}

// +--------------------------------------------------------------------+

void
SeekerAI::Navigator()
{
    if (delay > 0) {
        delay -= Game::FrameTime();
    }
    else {
        Steer s = SeekTarget();
        self->ApplyYaw((float)s.yaw);
        self->ApplyPitch((float)s.pitch);
    }
}

void
SeekerAI::SetTarget(SimObject* targ, SimSystem* sub)
{
    if (!orig_target && targ && targ->Type() == SimObject::SIM_SHIP) {
        orig_target = (Ship*)targ;
        Observe(orig_target);
    }

    SteerAI::SetTarget(targ, sub);

    if (!target) {
        shot->SetEta(0);

        if (shot->Owner())
            ((Ship*)shot->Owner())->SetMissileEta(shot->Identity(), 0);
    }
}

void
SeekerAI::FindObjective()
{
    if (!shot || !target)
        return;

    if (target->Life() == 0) {
        if (target != orig_target)
            SetTarget(orig_target, 0);
        else
            SetTarget(0, 0);

        return;
    }

    FVector TLoc = target->Location();
    TLoc = Transform(TLoc);

    // seeker head limit of 45 degrees:
    if (TLoc.Z < 0 || TLoc.Z < fabs(TLoc.X) || TLoc.Z < fabs(TLoc.Y)) {
        overshot = true;
        SetTarget(0, 0);
        return;
    }

    // distance from self to target:
    distance = FVector(target->Location() - self->Location()).Length();

    // are we being spoofed?
    CheckDecoys(distance);

    FVector Cv = ClosingVelocity();

    const double CvLen = Cv.Length();
    if (CvLen < 1e-3) {
        // Avoid divide-by-zero; treat as immediate intercept:
        obj_w = target->Location();
        objective = Transform(obj_w);
        objective.Normalize();
        shot->SetEta(0);

        if (shot->Owner())
            ((Ship*)shot->Owner())->SetMissileEta(shot->Identity(), 0);

        return;
    }

    // time to reach target:
    double time = distance / CvLen;
    double predict = time;
    if (predict > 15)
        predict = 15;

    // pure pursuit:
    if (pursuit == 1 || time < 0.1) {
        obj_w = target->Location();
    }

    // lead pursuit:
    else {
        // where the target will be when we reach it:
        FVector RunVec = target->Velocity();
        obj_w = target->Location() + (RunVec * predict);
    }

    // subsystem offset:
    if (subtarget) {
        FVector Offset = target->Location() - subtarget->MountLocation();
        obj_w -= Offset;
    }
    else if (target->Type() == SimObject::SIM_SHIP) {
        Ship* tgt_ship = (Ship*)target;

        if (tgt_ship->IsGroundUnit())
            obj_w += FVector(0, 150, 0);
    }

    distance = FVector(obj_w - self->Location()).Length();
    time = distance / CvLen;

    // where we will be when the target gets there:
    if (predict > 0.1 && predict < 15) {
        FVector SelfDest = self->Location() + Cv * predict;
        FVector Err = obj_w - SelfDest;

        obj_w += Err;
    }

    // transform into camera coords:
    objective = Transform(obj_w);
    objective.Normalize();

    shot->SetEta((int)time);

    if (shot->Owner())
        ((Ship*)shot->Owner())->SetMissileEta(shot->Identity(), (int)time);
}

// +--------------------------------------------------------------------+

void
SeekerAI::CheckDecoys(double target_distance)
{
    // if the assigned target has the burner lit,
    // ignore the decoys:
    if (orig_target && orig_target->Augmenter()) {
        SetTarget(orig_target);
        return;
    }

    if (target &&
        target == orig_target &&
        orig_target->GetActiveDecoys().size()) {

        ListIter<SimShot> decoy = orig_target->GetActiveDecoys();

        while (++decoy) {
            double decoy_distance = FVector(decoy->Location() - self->Location()).Length();

            if (decoy_distance < target_distance) {
                // Legacy behavior: rand() < 1600 (out of RAND_MAX).
                // UE replacement: low probability gate.
                if (FMath::RandRange(0, 32767) < 1600) {
                    SetTarget(decoy.value(), 0);
                    return;
                }
            }
        }
    }
}

// +--------------------------------------------------------------------+

bool
SeekerAI::Overshot()
{
    return overshot;
}

// +--------------------------------------------------------------------+

Steer
SeekerAI::AvoidCollision()
{
    return Steer();
}

// +--------------------------------------------------------------------+

Steer
SeekerAI::SeekTarget()
{
    if (!self || !target || Overshot())
        return Steer();

    return Seek(objective);
}

// +--------------------------------------------------------------------+

bool
SeekerAI::Update(SimObject* obj)
{
    if (obj == target) {
        if (obj->Type() == SimObject::SIM_SHOT && orig_target != 0)
            target = orig_target;
    }

    if (obj == orig_target)
        orig_target = 0;

    return SteerAI::Update(obj);
}

const char*
SeekerAI::GetObserverName() const
{
    static thread_local char NameBuf[64];

#if PLATFORM_WINDOWS
    _snprintf_s(NameBuf, sizeof(NameBuf), _TRUNCATE, "SeekerAI(%s)", self ? self->Name() : "null");
#else
    snprintf(NameBuf, sizeof(NameBuf), "SeekerAI(%s)", self ? self->Name() : "null");
#endif

    return NameBuf;
}
