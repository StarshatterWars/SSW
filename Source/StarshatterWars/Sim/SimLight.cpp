/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         SimLight.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Dynamic Light Source (UE port, plain C++)
*/

#include "SimLight.h"
#include "SimScene.h"

// Minimal Unreal support:
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimLight, Log, All);

// +--------------------------------------------------------------------+

int SimLight::id_key = 1;

// +--------------------------------------------------------------------+

SimLight::SimLight(float l, float dl, int time)
    : id(id_key++)
    , type(LIGHTTYPE::POINT)
    , loc(FVector::ZeroVector)
    , orient(FQuat::Identity)     // NEW: default orientation
    , life(time)
    , light(l)
    , dldt(dl)
    , color(255, 255, 255)
    , active(true)
    , shadow(false)
    , scene(nullptr)
{
}

// +--------------------------------------------------------------------+

SimLight::~SimLight()
{
}

// +--------------------------------------------------------------------+

void SimLight::Update()
{
    if (dldt < 1.0f)
        light *= dldt;

    if (life > 0)
        life--;
}

// +--------------------------------------------------------------------+

void SimLight::Destroy()
{
    if (scene)
        scene->DelLight(this);

    delete this;
}

// +--------------------------------------------------------------------+

void SimLight::MoveTo(const FVector& dst)
{
    // Directional lights can still have a loc (used by some tools),
    // but legacy code typically ignores it for directional.
    loc = dst;
}

void SimLight::TranslateBy(const FVector& ref)
{
    // Legacy behavior: do not translate directional lights by region offset
    if (type != LIGHTTYPE::DIRECTIONAL)
        loc = loc - ref;
}
