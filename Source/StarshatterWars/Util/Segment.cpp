/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Segment.cpp
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Segment = a contiguous run of polys sharing a material.
    Owns optional VideoPrivateData (renderer cache), nothing else.
*/

#include "Segment.h"

#include <cstring>

Segment::Segment()
    : npolys(0)
    , polys(nullptr)
    , material(nullptr)
    , model(nullptr)
    , video_data(nullptr)
{
}

Segment::Segment(int n, Poly* p, Material* mtl, SimModel* mod)
    : npolys(n)
    , polys(p)
    , material(mtl)
    , model(mod)
    , video_data(nullptr)
{
}

Segment::~Segment()
{
    delete video_data;
    video_data = nullptr;
}
