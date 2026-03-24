/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.

    SUBSYSTEM:    nGenEx.lib
    FILE:         SimProjector.h
*/

#pragma once

// Standard library (for sin/cos/atan/fabs):
#include <cmath>

// Starshatter core types (Rect, Matrix, Plane, etc):
#include "Types.h"
#include "Geometry.h"
#include "Polygon.h"

// Minimal Unreal types:
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h" // FMath

class View;
class Camera;

// NOTE:
// Plane is used as a member array -> must be a COMPLETE type here.
// If Plane is defined in Polygon.h/Geometry.h you're good.
// If not, include the header where Plane is actually defined.

class SimProjector
{
public:
    SimProjector(View* InWindow, Camera* InCamera);
    virtual ~SimProjector();

    // Operations:
    virtual void   UseWindow(View* win);
    virtual void   UseCamera(Camera* cam);
    virtual void   SetDepthScale(float scale);
    virtual double GetDepthScale() const;
    virtual void   SetFieldOfView(double fov);
    virtual double GetFieldOfView() const;
    virtual int    SetInfinite(int i);
    virtual void   StartFrame();

    // Accessors:
    FVector        Pos() const;
    FVector        vrt() const;
    FVector        vup() const;
    FVector        vpn() const;
    const Matrix& Orientation() const;

    double         XAngle() const { return xangle; }
    double         YAngle() const { return yangle; }

    bool           IsOrthogonal() const { return orthogonal; }
    void           SetOrthogonal(bool o) { orthogonal = o; }

    // Projection and clipping geometry:
    virtual void   Transform(FVector& vec) const;

    virtual void   Project(FVector& vec, bool clamp = true) const;
    virtual void   ProjectRect(FVector& origin, double& w, double& h) const;

    virtual float  ProjectRadius(const FVector& vec, float radius) const;

    virtual void   Unproject(FVector& point) const;
    int            IsVisible(const FVector& v, float radius) const;
    int            IsBoxVisible(const FVector* p) const;

    float          ApparentRadius(const FVector& v, float radius) const;

    virtual void   SetWorldSpace() { frustum_planes = world_planes; }
    virtual void   SetViewSpace() { frustum_planes = view_planes; }

    Plane* GetCurrentClipPlanes() { return frustum_planes; }

    void           SetUpFrustum();
    void           ViewToWorld(FVector& pin, FVector& pout);
    void           SetWorldspaceClipPlane(FVector& normal, Plane& plane);

protected:
    Camera* camera = nullptr;

    int            width = 0;
    int            height = 0;

    double         field_of_view = 2.0;
    double         xscreenscale = 0.0;
    double         yscreenscale = 0.0;
    double         maxscale = 0.0;
    double         xcenter = 0.0;
    double         ycenter = 0.0;
    double         xangle = 0.0;
    double         yangle = 0.0;

    int            infinite = 0;
    float          depth_scale = 1.0f;
    bool           orthogonal = false;

    enum DISPLAY_CONST { NUM_FRUSTUM_PLANES = 4 };

    Plane* frustum_planes = nullptr;
    Plane          world_planes[NUM_FRUSTUM_PLANES];
    Plane          view_planes[NUM_FRUSTUM_PLANES];

    float          xclip0 = 0.0f;
    float          xclip1 = 0.0f;
    float          yclip0 = 0.0f;
    float          yclip1 = 0.0f;
};
