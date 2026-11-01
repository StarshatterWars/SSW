/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGen.lib
    FILE:         Director.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR / STUDIO
    ========================
    John DiCamillo / Destroyer Studios LLC (Starshatter / nGen)

    OVERVIEW
    ========
    Abstract Director (AI or Human Input) for Physical Objects
*/

#pragma once
#include "CoreMinimal.h"
#include "Types.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

class Physical;

// +--------------------------------------------------------------------+

class SimDirector
{
public:
    SimDirector() = default;
    virtual ~SimDirector() = default;

    // accessors:
    virtual ESteerAIType GetType()     const { return ESteerAIType::NONE; }
    // Keep the legacy virtual contract behind the Unreal accessor.
    virtual int          Subframe() const { return false; }
    virtual bool         GetSubframe() const { return Subframe() != 0; }

    // operations:
    virtual void ExecFrame(double factor) {}
};
