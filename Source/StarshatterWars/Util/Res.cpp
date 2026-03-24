/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    Original Author and Studio: John DiCamillo, Destroyer Studios LLC
    AUTHOR:       Carlos Bott

    SUBSYSTEM:    nGenEx.lib
    FILE:         Res.cpp

    OVERVIEW
    ========
    Abstract Resource class
*/

#include "Res.h"

// +--------------------------------------------------------------------+

static int RESOURCE_KEY = 1;

Resource::Resource()
    : id(reinterpret_cast<HANDLE>(RESOURCE_KEY++))
{
}

Resource::~Resource()
{
}
