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

// +--------------------------------------------------------------------+

class Physical;

// +--------------------------------------------------------------------+

class SimDirector
{
public:
    SimDirector() = default;
    virtual ~SimDirector() = default;

    // accessors:
    virtual int Type()     const { return 0; }
    virtual int Subframe() const { return 0; }

    // operations:
    virtual void ExecFrame(double factor) {}
};
