/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         QuantumFlash.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Quantum Warp Out special effect class
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Graphic.h"
#include "Polygon.h"
#include "SimObject.h"

// Minimal Unreal includes only where necessary:
#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // Math

// +--------------------------------------------------------------------+

class Video;
struct Material;
struct VertexSet;
struct Poly;
class Bitmap;

// +--------------------------------------------------------------------+

class QuantumFlash : public Graphic
{
public:
    QuantumFlash();
    virtual ~QuantumFlash();

    // operations
    virtual void   Render(Video* video, DWORD flags);

    // accessors / mutators
    virtual void   SetOrientation(const Matrix& o);

    // Converted core vectors/points to FVector:
    void           SetDirection(const FVector& v);
    void           SetEndPoints(const FVector& from, const FVector& to);

    double         Shade() const { return shade; }
    void           SetShade(double s);

protected:
    void           UpdateVerts(const FVector& cam_pos);

    double         length;
    double         width;
    double         shade;

    int            npolys, nverts;
    Material* mtl;
    VertexSet* verts;
    Poly* polys;
    Matrix* beams;
    Bitmap* texture;

    Matrix         orientation;
};
