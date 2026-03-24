/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Bolt.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    3D Bolt (Polygon) Object
*/

#pragma once

#include "Graphic.h"
#include "Polygon.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+
// Forward Declarations
// +--------------------------------------------------------------------+

class Video;
class Bitmap;

// +--------------------------------------------------------------------+

class Bolt : public Graphic
{
public:
    static const char* TYPENAME() { return "Bolt"; }

    Bolt(double len = 16, double wid = 1, Bitmap* tex = 0, int share = 0);
    virtual ~Bolt();

    // operations
    virtual void   Render(Video* video, DWORD flags);
    virtual void   Update();

    // accessors / mutators
    virtual void   SetOrientation(const Matrix& o);
    void           SetDirection(const FVector& v);
    void           SetEndPoints(const FVector& from, const FVector& to);
    void           SetTextureOffset(double from, double to);

    virtual void   TranslateBy(const FVector& ref);

    double         Shade()        const { return shade; }
    void           SetShade(double s) { shade = s; }
    virtual bool   IsBolt()       const { return true; }

protected:
    double         length = 0.0;
    double         width = 0.0;
    double         shade = 0.0;

    Poly           poly;
    Material       mtl;
    VertexSet      vset;
    Bitmap* texture = nullptr;
    int            shared = 0;

    FVector        vpn = FVector::ZeroVector;
    FVector        origin = FVector::ZeroVector;
};
