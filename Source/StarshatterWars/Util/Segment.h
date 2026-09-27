/*  Starshatter Wars
    Fractal Dev Studios
    Copyright(C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Segment.h
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Segment: cohesive run of polys sharing a material
*/

#pragma once

#include "Polygon.h"
#include "Video.h"

// Forward declarations:
class SimModel;

class Segment
{
public:
    static const char* TYPENAME() { return "Segment"; }

    Segment();
    Segment(int n, Poly* p, Material* mtl, SimModel* mod = nullptr);
    ~Segment();

    bool              IsSolid()       const { return material ? material->IsSolid() : true; }
    bool              IsTranslucent() const { return material ? material->IsTranslucent() : false; }
    bool              IsGlowing()     const { return material ? material->IsGlowing() : false; }

    VideoPrivateData* GetVideoPrivateData() const { return video_data; }
    void              SetVideoPrivateData(VideoPrivateData* vpd) { video_data = vpd; }

    int               npolys;
    Poly* polys;
    Material* material;
    SimModel* model;
    VideoPrivateData* video_data;
};
