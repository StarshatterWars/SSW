/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    Original Author and Studio: John DiCamillo, Destroyer Studios LLC
    AUTHOR:       Carlos Bott

    SUBSYSTEM:    nGenEx.lib
    FILE:         Res.h

    OVERVIEW
    ========
    Abstract Resource class
*/

#pragma once

#include "Types.h"

// +--------------------------------------------------------------------+

class Resource
{
public:
    Resource();
    virtual ~Resource();

    int operator == (const Resource& r) const { return id == r.id; }

    HANDLE Handle() const { return id; }

protected:
    HANDLE id;
};

