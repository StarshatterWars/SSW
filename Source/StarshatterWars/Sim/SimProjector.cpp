/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         SimProjector.cpp
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    3D Projection Camera class
*/

#include "SimProjector.h"

// Starshatter core:
#include "Camera.h"
#include "View.h"

#include "Logging/LogMacros.h"
#include <cmath> // std::sin/cos/atan/fabs

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimProjector, Log, All);

// +--------------------------------------------------------------------+

static const float   CLIP_PLANE_EPSILON = 0.0001f;
static const double  Z_NEAR = 1.0;

// Emergency fallback camera:
static Camera emergency_cam;

// +--------------------------------------------------------------------+

SimProjector::SimProjector(View* InWindow, Camera* InCamera)
    : camera(InCamera)
{
    if (!camera)
        camera = &emergency_cam;

    // init defaults (also present in header in most ports):
    infinite = 0;
    depth_scale = 1.0f;
    orthogonal = false;

    width = 0;
    height = 0;

    field_of_view = 2.0;
    xscreenscale = 0.0;
    yscreenscale = 0.0;
    maxscale = 0.0;
    xcenter = 0.0;
    ycenter = 0.0;
    xangle = 0.0;
    yangle = 0.0;

    frustum_planes = world_planes;

    xclip0 = 0.0f; xclip1 = 0.0f;
    yclip0 = 0.0f; yclip1 = 0.0f;

    UseWindow(InWindow);
}

SimProjector::~SimProjector()
{
}

// +--------------------------------------------------------------------+

void SimProjector::UseCamera(Camera* cam)
{
    camera = cam ? cam : &emergency_cam;
}

void SimProjector::UseWindow(View* win)
{
    if (!win) {
        width = height = 0;
        xcenter = ycenter = 0.0;
        xscreenscale = yscreenscale = maxscale = 0.0;
        xangle = yangle = 0.0;
        xclip0 = yclip0 = 0.0f;
        xclip1 = yclip1 = 0.0f;
        return;
    }

    Rect r = win->GetRect();
    width = r.w;
    height = r.h;

    xcenter = (width / 2.0);
    ycenter = (height / 2.0);

    xclip0 = 0.0f;
    xclip1 = (float)width - 0.5f;
    yclip0 = 0.0f;
    yclip1 = (float)height - 0.5f;

    SetFieldOfView(field_of_view);
}

// +--------------------------------------------------------------------+

void SimProjector::SetDepthScale(float scale)
{
    depth_scale = scale;
}

double SimProjector::GetDepthScale() const
{
    return (double)depth_scale;
}

// +--------------------------------------------------------------------+

void SimProjector::SetFieldOfView(double fov)
{
    // Guard against silly values:
    if (fov < 0.001)
        fov = 0.001;

    field_of_view = fov;

    // Legacy Starshatter formula:
    xscreenscale = (width > 0) ? (width / fov) : 0.0;
    yscreenscale = (height > 0) ? (height / fov) : 0.0;

    maxscale = FMath::Max(xscreenscale, yscreenscale);

    // Avoid divide-by-zero:
    const double xs = (xscreenscale != 0.0) ? xscreenscale : 1.0;
    const double ys = (yscreenscale != 0.0) ? yscreenscale : 1.0;

    xangle = std::atan(2.0 / fov * maxscale / xs);
    yangle = std::atan(2.0 / fov * maxscale / ys);
}

double SimProjector::GetFieldOfView() const
{
    return field_of_view;
}

// +--------------------------------------------------------------------+

int SimProjector::SetInfinite(int i)
{
    const int old = infinite;
    infinite = i;
    return old;
}

// +--------------------------------------------------------------------+

void SimProjector::StartFrame()
{
    SetUpFrustum();
    SetWorldSpace();
}

// +--------------------------------------------------------------------+
// Accessors
// +--------------------------------------------------------------------+

FVector SimProjector::Pos() const
{
    const auto P = camera->Pos();
    return FVector((float)P.X, (float)P.Y, (float)P.Z);
}

FVector SimProjector::vrt() const
{
    const auto V = camera->vrt();
    return FVector((float)V.X, (float)V.Y, (float)V.Z);
}

FVector SimProjector::vup() const
{
    const auto V = camera->vup();
    return FVector((float)V.X, (float)V.Y, (float)V.Z);
}

FVector SimProjector::vpn() const
{
    const auto V = camera->vpn();
    return FVector((float)V.X, (float)V.Y, (float)V.Z);
}

const Matrix& SimProjector::Orientation() const
{
    return camera->Orientation();
}

// +--------------------------------------------------------------------+
// Transform a point from worldspace to viewspace.
// +--------------------------------------------------------------------+

void SimProjector::Transform(FVector& vec) const
{
    FVector tvert = vec;

    if (!infinite) {
        const FVector CamPos = Pos();
        tvert -= CamPos;
    }

    const FVector Vrt = vrt();
    const FVector Vup = vup();
    const FVector Vpn = vpn();

    vec.X = FVector::DotProduct(tvert, Vrt);
    vec.Y = FVector::DotProduct(tvert, Vup);
    vec.Z = FVector::DotProduct(tvert, Vpn);
}

// +--------------------------------------------------------------------+
// Radius projection
// +--------------------------------------------------------------------+

float SimProjector::ProjectRadius(const FVector& v, float radius) const
{
    if (v.Z == 0.0f)
        return 0.0f;

    return (float)std::fabs((radius * maxscale) / (double)v.Z);
}

float SimProjector::ApparentRadius(const FVector& v, float radius) const
{
    FVector vloc = v;
    Transform(vloc);
    return ProjectRadius(vloc, radius);
}

// +--------------------------------------------------------------------+
// In-place project/unproject
// +--------------------------------------------------------------------+

void SimProjector::Project(FVector& v, bool clamp) const
{
    if (width <= 0 || height <= 0)
        return;

    if (orthogonal) {
        const double scale = field_of_view / 2.0;
        v.X = (float)(xcenter + scale * v.X);
        v.Y = (float)(height - (ycenter + scale * v.Y));
        v.Z = 0.0f;
    }
    else {
        // Avoid blowups:
        if (v.Z == 0.0f)
            v.Z = 0.0001f;

        const double zrecip = 2.0 / (double)v.Z;
        v.X = (float)(xcenter + maxscale * v.X * zrecip);
        v.Y = (float)(height - (ycenter + maxscale * v.Y * zrecip));
        v.Z = (float)(1.0 - zrecip);
    }

    if (clamp) {
        if (v.X < xclip0) v.X = xclip0;
        if (v.X > xclip1) v.X = xclip1;
        if (v.Y < yclip0) v.Y = yclip0;
        if (v.Y > yclip1) v.Y = yclip1;
    }
}

void SimProjector::Unproject(FVector& v) const
{
    if (width <= 0 || height <= 0)
        return;

    if (v.Z == 0.0f)
        v.Z = 0.0001f;

    const double zrecip = 1.0 / (double)v.Z;

    v.X = (float)((v.X - xcenter) / (maxscale * zrecip));
    v.Y = (float)((height - v.Y - ycenter) / (maxscale * zrecip));
}

// +--------------------------------------------------------------------+
// In-place projection of rectangle for sprites
// +--------------------------------------------------------------------+

void SimProjector::ProjectRect(FVector& v, double& w, double& h) const
{
    if (width <= 0 || height <= 0)
        return;

    if (orthogonal) {
        const double scale = field_of_view / 2.0;
        v.X = (float)(xcenter + scale * v.X);
        v.Y = (float)(height - (ycenter + scale * v.Y));
        v.Z = 0.0f;
    }
    else {
        if (v.Z == 0.0f)
            v.Z = 0.0001f;

        const double zrecip = 1.0 / (double)v.Z;

        v.X = (float)(xcenter + 2.0 * maxscale * v.X * zrecip);
        v.Y = (float)(height - (ycenter + 2.0 * maxscale * v.Y * zrecip));
        v.Z = (float)(1.0 - Z_NEAR * zrecip);

        w *= maxscale * zrecip;
        h *= maxscale * zrecip;
    }
}

// +--------------------------------------------------------------------+
// Frustum setup / clip planes
// +--------------------------------------------------------------------+

void SimProjector::SetWorldspaceClipPlane(FVector& normal, Plane& plane)
{
    // Rotate plane normal into worldspace
    FVector worldNormal = FVector::ZeroVector;
    ViewToWorld(normal, worldNormal);

    plane.normal.X = worldNormal.X;
    plane.normal.Y = worldNormal.Y;
    plane.normal.Z = worldNormal.Z;

    const FVector camPos = Pos();
    plane.distance = (float)(
        camPos.X * plane.normal.X +
        camPos.Y * plane.normal.Y +
        camPos.Z * plane.normal.Z +
        CLIP_PLANE_EPSILON
        );
}

void SimProjector::SetUpFrustum()
{
    double  angle = 0.0, s = 0.0, c = 0.0;
    FVector normal = FVector::ZeroVector;

    // Left/Right planes from xangle:
    angle = XAngle();
    s = std::sin(angle);
    c = std::cos(angle);

    // Left
    normal.X = (float)s;
    normal.Y = 0.0f;
    normal.Z = (float)c;
    view_planes[0].normal.X = normal.X;
    view_planes[0].normal.Y = normal.Y;
    view_planes[0].normal.Z = normal.Z;
    view_planes[0].distance = CLIP_PLANE_EPSILON;
    SetWorldspaceClipPlane(normal, world_planes[0]);

    // Right
    normal.X = (float)-s;
    view_planes[1].normal.X = normal.X;
    view_planes[1].normal.Y = normal.Y;
    view_planes[1].normal.Z = normal.Z;
    view_planes[1].distance = CLIP_PLANE_EPSILON;
    SetWorldspaceClipPlane(normal, world_planes[1]);

    // Bottom/Top planes from yangle:
    angle = YAngle();
    s = std::sin(angle);
    c = std::cos(angle);

    // Bottom
    normal.X = 0.0f;
    normal.Y = (float)s;
    normal.Z = (float)c;
    view_planes[2].normal.X = normal.X;
    view_planes[2].normal.Y = normal.Y;
    view_planes[2].normal.Z = normal.Z;
    view_planes[2].distance = CLIP_PLANE_EPSILON;
    SetWorldspaceClipPlane(normal, world_planes[2]);

    // Top
    normal.Y = (float)-s;
    view_planes[3].normal.X = normal.X;
    view_planes[3].normal.Y = normal.Y;
    view_planes[3].normal.Z = normal.Z;
    view_planes[3].distance = CLIP_PLANE_EPSILON;
    SetWorldspaceClipPlane(normal, world_planes[3]);

    frustum_planes = world_planes;
}

// +--------------------------------------------------------------------+
// Visibility tests
// +--------------------------------------------------------------------+

int SimProjector::IsVisible(const FVector& v, float radius) const
{
    int visible = 1;
    int complete = 1;

    const Plane* plane = frustum_planes;

    if (infinite) {
        complete = 0;
        for (int i = 0; visible && (i < NUM_FRUSTUM_PLANES); ++i, ++plane) {
            const float dot = FVector::DotProduct(v, plane->normal);
            visible = (dot >= CLIP_PLANE_EPSILON);
        }
    }
    else {
        for (int i = 0; visible && (i < NUM_FRUSTUM_PLANES); ++i, ++plane) {
            const float dot = FVector::DotProduct(v, plane->normal);
            visible = ((dot + radius) >= plane->distance);
            complete = complete && ((dot - radius) >= plane->distance);
        }
    }

    // 0 = not visible, 1 = partial, 2 = complete
    return visible + complete;
}

int SimProjector::IsBoxVisible(const FVector* p) const
{
    if (!p)
        return 0;

    int outside = 0;
    const Plane* plane = frustum_planes;

    if (infinite) {
        for (int i = 0; (outside == 0) && (i < NUM_FRUSTUM_PLANES); ++i, ++plane) {
            int count = 0;
            for (int j = 0; j < 8; ++j) {
                const float dot = FVector::DotProduct(p[j], plane->normal);
                if (dot < CLIP_PLANE_EPSILON)
                    ++count;
            }
            if (count == 8)
                outside = 1;
        }
    }
    else {
        for (int i = 0; (outside == 0) && (i < NUM_FRUSTUM_PLANES); ++i, ++plane) {
            int count = 0;
            for (int j = 0; j < 8; ++j) {
                const float dot = FVector::DotProduct(p[j], plane->normal);
                if (dot < plane->distance)
                    ++count;
            }
            if (count == 8)
                outside = 1;
        }
    }

    return outside ? 0 : 1;
}

// +--------------------------------------------------------------------+
// Viewspace -> Worldspace rotation
// +--------------------------------------------------------------------+

void SimProjector::ViewToWorld(FVector& vin, FVector& vout)
{
    const FVector Vrt = vrt();
    const FVector Vup = vup();
    const FVector Vpn = vpn();

    vout.X = vin.X * Vrt.X + vin.Y * Vup.X + vin.Z * Vpn.X;
    vout.Y = vin.X * Vrt.Y + vin.Y * Vup.Y + vin.Z * Vpn.Y;
    vout.Z = vin.X * Vrt.Z + vin.Y * Vup.Z + vin.Z * Vpn.Z;
}
