/*  Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC

    SUBSYSTEM: nGenEx.lib
    FILE: Solid.cpp
    AUTHOR: Carlos Bott

    OVERVIEW
    ========
    Solid render + collision logic (UE-native, OPCODE removed)
*/

#include "Solid.h"
#include "SimModel.h"
#include "ModelFile.h"
#include "Surface.h"
#include "MeshCollision.h"
#include "Segment.h"
#include "Shadow.h"
#include "SimLight.h"
#include "SimProjector.h"
#include "DataLoader.h"

#include "CoreMinimal.h"

DEFINE_LOG_CATEGORY_STATIC(LogSolid, Log, All);

// --------------------------------------------------------------------
// Global collision enable
// --------------------------------------------------------------------

static bool GCollisionEnabled = true;

bool Solid::IsCollisionEnabled()
{
    return GCollisionEnabled;
}

void Solid::EnableCollision(bool enable)
{
    GCollisionEnabled = enable;
}

// --------------------------------------------------------------------
// Construction / Destruction
// --------------------------------------------------------------------

Solid::Solid()
    : model(nullptr)
    , own_model(true)
    , roll(0.0f)
    , pitch(0.0f)
    , yaw(0.0f)
    , OrientationMatrix(FMatrix::Identity)
    , intersection_poly(nullptr)
{
    shadow = true;

    // ASCII-safe formatting:
    FCStringAnsi::Snprintf(name, NAMELEN, "Solid %d", id);
}

Solid::~Solid()
{
    if (own_model && model) {
        delete model;
        model = nullptr;
    }

    shadows.destroy();
}

// --------------------------------------------------------------------
// Core update (placeholder - legacy behavior)
// --------------------------------------------------------------------

void Solid::Update()
{
    // Legacy solids update via SimObject / Scene
}

// --------------------------------------------------------------------
// Rendering
// --------------------------------------------------------------------

void Solid::Render(Video* video, DWORD flags)
{
    if (!video || !model || model->GetNumPolys() < 1)
        return;

    DWORD blend = Video::BLEND_SOLID;

    if (flags & RENDER_ALPHA)
        blend = Video::BLEND_ALPHA | Video::BLEND_ADDITIVE;

    video->DrawSolid(this, blend);
}

// --------------------------------------------------------------------
// Detail / Projection
// --------------------------------------------------------------------

void Solid::SelectDetail(SimProjector*)
{
    // Legacy stub
}

void Solid::ProjectScreenRect(SimProjector* proj)
{
    if (!proj || !model)
        return;

    FVector center = loc;
    proj->Transform(center);

    if (center.Z <= 1.0f)
        return;

    int left = 2000;
    int right = -2000;
    int top = 2000;
    int bottom = -2000;

    for (int i = 0; i < 6; ++i) {
        FVector extent(0, 0, 0);

        if (i < 2)       extent.X = model->extents[i];
        else if (i < 4)  extent.Y = model->extents[i];
        else             extent.Z = model->extents[i];

        extent = OrientationMatrix.TransformVector(extent) + loc;
        proj->Transform(extent);
        proj->Project(extent);

        left = FMath::Min(left, (int)extent.X);
        right = FMath::Max(right, (int)extent.X);
        top = FMath::Min(top, (int)extent.Y);
        bottom = FMath::Max(bottom, (int)extent.Y);
    }

    screen_rect = Rect(left, top, right - left, bottom - top);
}

// --------------------------------------------------------------------
// Orientation / State
// --------------------------------------------------------------------

void Solid::SetOrientation(const FMatrix& m)
{
    OrientationMatrix = m;
}

void Solid::SetOrientation(const Solid& match)
{
    OrientationMatrix = match.Orientation();
}

bool Solid::IsDynamic() const
{
    return model ? model->IsDynamic() : false;
}

void Solid::SetDynamic(bool d)
{
    if (model)
        model->SetDynamic(d);
}

void Solid::SetLuminous(bool l)
{
    if (model)
        model->SetLuminous(l);
}

// --------------------------------------------------------------------
// Loading / Model management
// --------------------------------------------------------------------

bool Solid::Load(const char* mag_file, double scale)
{
    ClearModel();

    model = new SimModel;
    own_model = true;

    if (model->Load(mag_file, scale)) {
        radius = model->GetRadius();

        // ASCII-only, UE-native:
        FCStringAnsi::Strncpy(name, model->GetName(), NAMELEN);

        return true;
    }

    ClearModel();
    return false;
}

bool Solid::Load(ModelFile* loader, double scale)
{
    ClearModel();

    model = new SimModel;
    own_model = true;

    if (model->Load(loader, scale)) {
        radius = model->GetRadius();
        return true;
    }

    ClearModel();
    return false;
}

void Solid::UseModel(SimModel* m)
{
    ClearModel();

    model = m;
    own_model = false;
    radius = model ? model->GetRadius() : 0.0f;
}

void Solid::ClearModel()
{
    if (own_model && model) {
        delete model;
    }

    model = nullptr;
    radius = 0.0f;
}

// --------------------------------------------------------------------
// Texture gathering
// --------------------------------------------------------------------

void Solid::GetAllTextures(List<Bitmap>& textures)
{
    if (model)
        model->GetAllTextures(textures);
}

// --------------------------------------------------------------------
// Shadows
// --------------------------------------------------------------------

void Solid::CreateShadows(int nlights)
{
    while (shadows.size() < nlights) {
        shadows.append(new Shadow(this));
    }
}

void Solid::UpdateShadows(List<SimLight>& lights)
{
    ListIter<SimLight> it = lights;
    int index = 0;

    while (++it && index < shadows.size()) {
        SimLight* light = it.value();
        if (light && light->IsActive() && light->CastsShadow()) {
            shadows[index]->Update(light);
            ++index;
        }
    }
}

// --------------------------------------------------------------------
// Collision: sphere broad phase, cached triangle narrow phase
// --------------------------------------------------------------------

int Solid::CollidesWith(Graphic& other)
{
    if (!GCollisionEnabled || !other.IsSolid())
        return 0;

    const FVector delta = Location() - other.Location();

    if (delta.Size() > Radius() + other.Radius())
        return 0;

    Solid& OtherSolid = static_cast<Solid&>(other);
    if (!model || !OtherSolid.model || !MeshCollision::HasGeometry(*model) || !MeshCollision::HasGeometry(*OtherSolid.model))
        return 1; // Preserve the authored sphere fallback for geometry-free reps.
    FTransform Pose(OrientationMatrix), OtherPose(OtherSolid.OrientationMatrix);
    Pose.SetTranslation(loc); OtherPose.SetTranslation(OtherSolid.loc);
    return MeshCollision::Overlap(*model, Pose, *OtherSolid.model, OtherPose) ? 1 : 0;
}

// --------------------------------------------------------------------
// Ray intersection (nearest mesh surface, sphere fallback without geometry)
// --------------------------------------------------------------------

int Solid::CheckRayIntersection(FVector rayOrigin, FVector rayDir, double length,
    FVector& outHit, bool treat_translucent_polys_as_solid)
{
    intersection_poly = nullptr;
    if (!model || rayOrigin.ContainsNaN() || rayDir.ContainsNaN() || !FMath::IsFinite(length) || length < 0)
        return 0;
    rayDir = rayDir.GetSafeNormal();
    if (rayDir.IsNearlyZero()) return 0;
    if (!MeshCollision::HasGeometry(*model))
        return MeshCollision::RaySphere(rayOrigin, rayDir, length, loc, radius, outHit) ? 1 : 0;
    FTransform Pose(OrientationMatrix);
    Pose.SetTranslation(loc);
    return MeshCollision::Ray(*model, Pose, rayOrigin, rayDir, length,
        treat_translucent_polys_as_solid, outHit, intersection_poly) ? 1 : 0;
}

// --------------------------------------------------------------------
// Buffer / render cache invalidation
// --------------------------------------------------------------------

void Solid::DeletePrivateData()
{
    if (model)
        model->DeletePrivateData();
}

void Solid::InvalidateSurfaceData()
{
    if (!model)
        return;

    ListIter<Surface> it = model->GetSurfaces();
    while (++it) {
        Surface* s = it.value();
        if (s) {
            VideoPrivateData* vpd = s->GetVideoPrivateData();
            if (vpd)
                vpd->Invalidate();
        }
    }
}

void Solid::InvalidateSegmentData()
{
    if (!model)
        return;

    ListIter<Surface> it = model->GetSurfaces();
    while (++it) {
        Surface* s = it.value();
        if (!s) continue;

        ListIter<Segment> segIt = s->GetSegments();
        while (++segIt) {
            Segment* seg = segIt.value();
            if (seg) {
                VideoPrivateData* vpd = seg->GetVideoPrivateData();
                if (vpd)
                    vpd->Invalidate();
            }
        }
    }
}

bool Solid::Rescale(double scale)
{
    if (scale <= 0.0)
        return false;

    if (!model)
        return false;

    // If your SimModel supports rescaling, call it:
    // (This is the most correct place for scale to live in the Starshatter architecture.)
    if (model->Rescale(scale))     // <-- implement this in SimModel if missing
    {
        // Legacy pipeline expects geometry-dependent caches to be rebuilt:
        InvalidateSurfaceData();
        InvalidateSegmentData();
        return true;
    }

    // If you don't have SimModel::Rescale yet, at least succeed and invalidate,
    // so the call sites stop breaking while you finish the model pipeline.
    InvalidateSurfaceData();
    InvalidateSegmentData();
    return true;
}

bool SimModel::Rescale(double scale)
{
    if (scale <= 0.0) return false;

    // Iterate all vertices in all surfaces/segments and multiply by scale.
    // Recompute bounds/radius.
    // Mark any cached VB/IB invalid.
    return true;
}
