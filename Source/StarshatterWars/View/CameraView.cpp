/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         CameraView.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC
    Copyright (C) 1997–2004.

    OVERVIEW
    ========
    CameraView implementation.

    Uses legacy Video/Projector/Scene pipeline. This class does not own
    rendering resources; it coordinates view rect, projection settings,
    visibility, sorting, and optional lens flare.
*/
#pragma once

#include "CameraView.h"

#include "Camera.h"
#include "SimScene.h"
#include "SimLight.h"
#include "SimProjector.h"
#include "Solid.h"
#include "Shadow.h"
#include "Video.h"
#include "Bitmap.h"
#include "Screen.h"

#include "Math/Color.h"

DEFINE_LOG_CATEGORY_STATIC(LogCameraView, Log, All);

// --------------------------------------------------------------------
// Emergency fallbacks (legacy behavior):
static Camera  emergency_vcam;
static SimScene emergency_vscene;

CameraView* CameraView::camera_view = nullptr;

// --------------------------------------------------------------------

CameraView::CameraView(Screen* InScreen, int ax, int ay, int aw, int ah, Camera* cam, SimScene* s)
    : View(InScreen, ax, ay, aw, ah)
    , camera(cam)
    , scene(s)
    , infinite(0)
    , projection_type(Video::PROJECTION_PERSPECTIVE)
{
    if (!camera)
        camera = &emergency_vcam;

    if (!scene)
        scene = &emergency_vscene;

    width = aw;
    height = ah;

    // IMPORTANT: Build projector AFTER camera is guaranteed valid:
    Projector = new SimProjector(GetWindow(), camera);

    // Keep projector state consistent:
    Projector->SetInfinite(infinite);

    OnWindowMove();
}

CameraView::~CameraView()
{
    delete Projector;
    Projector = nullptr;
}

// --------------------------------------------------------------------
// Convenience pass-throughs
// --------------------------------------------------------------------

FVector CameraView::Pos() const
{
    return camera ? camera->Pos() : FVector::ZeroVector;
}

FVector CameraView::vrt() const
{
    return camera ? camera->vrt() : FVector::UnitX();
}

FVector CameraView::vup() const
{
    return camera ? camera->vup() : FVector::UnitY();
}

FVector CameraView::vpn() const
{
    return camera ? camera->vpn() : FVector::UnitZ();
}

const Matrix& CameraView::Orientation() const
{
    return camera->Orientation();
}

// --------------------------------------------------------------------

void CameraView::UseCamera(Camera* cam)
{
    camera = cam ? cam : &emergency_vcam;

    if (Projector)
        Projector->UseCamera(camera);
}

void CameraView::UseScene(SimScene* s)
{
    scene = s ? s : &emergency_vscene;
}

void CameraView::SetFieldOfView(double fov)
{
    if (Projector)
        Projector->SetFieldOfView(fov);
}

double CameraView::GetFieldOfView() const
{
    return Projector ? Projector->GetFieldOfView() : 2.0;
}

void CameraView::SetProjectionType(uint32 pt)
{
    if (Projector)
        Projector->SetOrthogonal(pt == Video::PROJECTION_ORTHOGONAL);

    projection_type = pt;
}

uint32 CameraView::GetProjectionType() const
{
    return projection_type;
}

void CameraView::SetDepthScale(float scale)
{
    if (Projector)
        Projector->SetDepthScale(scale);
}

void CameraView::OnWindowMove()
{
    const Rect r = GetRect();
    width = r.w;
    height = r.h;

    if (Projector)
        Projector->UseWindow(GetWindow());
}

// --------------------------------------------------------------------
// Lens flare controls
// --------------------------------------------------------------------

void CameraView::LensFlare(int on, double dim)
{
    lens_flare_enable = on;
    lens_flare_dim = dim;
}

void CameraView::LensFlareElements(Bitmap* halo, Bitmap* e1, Bitmap* e2, Bitmap* e3)
{
    if (halo) halo_bitmap = halo;
    if (e1)   elem_bitmap[0] = e1;
    if (e2)   elem_bitmap[1] = e2;
    if (e3)   elem_bitmap[2] = e3;
}

// --------------------------------------------------------------------

int CameraView::SetInfinite(int i)
{
    const int old = infinite;
    infinite = i;

    if (Projector)
        Projector->SetInfinite(i);

    return old;
}

// --------------------------------------------------------------------
// Depth computation
// --------------------------------------------------------------------

void CameraView::FindDepth(Graphic* g)
{
    if (!g || !camera)
        return;

    if (infinite)
    {
        g->SetDepth(1.0e20f);
        return;
    }

    const FVector loc = g->Location() - camera->Pos();
    const float   z = FVector::DotProduct(loc, camera->vpn());

    g->SetDepth(z);
}

// --------------------------------------------------------------------

void CameraView::Refresh()
{
    // Disabled / emergency camera:
    if (camera == &emergency_vcam)
        return;

    video = Video::GetInstance();
    if (!video)
    {
        UE_LOG(LogCameraView, Warning, TEXT("CameraView::Refresh - Video::GetInstance() returned null"));
        return;
    }

    const Rect r = GetRect();
    width = r.w;
    height = r.h;

    cvrt = camera->vrt();
    cvup = camera->vup();
    cvpn = camera->vpn();

    TranslateScene();
    MarkVisibleObjects();

    Rect old_rect;
    video->GetWindowRect(old_rect);

    video->SetCamera(camera);
    video->SetWindowRect(r);
    video->SetProjection((float)GetFieldOfView(), 1.0f, 1.0e6f, projection_type);

    RenderBackground();
    RenderScene();
    RenderForeground();
    RenderSprites();
    RenderLensFlare();

    UnTranslateScene();

    video->SetWindowRect(old_rect);
}

// --------------------------------------------------------------------
// Translate scene to camera-relative coordinates
// --------------------------------------------------------------------

void CameraView::TranslateScene()
{
    camera_loc = camera->Pos();

    ListIter<Graphic> g_iter = scene->Graphics();
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic && !graphic->IsInfinite())
            graphic->TranslateBy(camera_loc);
    }

    g_iter.attach(scene->Foreground());
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic)
            graphic->TranslateBy(camera_loc);
    }

    g_iter.attach(scene->Sprites());
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic && !graphic->IsInfinite())
            graphic->TranslateBy(camera_loc);
    }

    ListIter<SimLight> l_iter = scene->Lights();
    while (++l_iter)
    {
        SimLight* light = l_iter.value();
        if (light)
            light->TranslateBy(camera_loc);
    }

    camera->MoveTo(0, 0, 0);
}

void CameraView::UnTranslateScene()
{
    const FVector reloc = -camera_loc;

    ListIter<Graphic> g_iter = scene->Graphics();
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic && !graphic->IsInfinite())
            graphic->TranslateBy(reloc);
    }

    g_iter.attach(scene->Foreground());
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic)
            graphic->TranslateBy(reloc);
    }

    g_iter.attach(scene->Sprites());
    while (++g_iter)
    {
        Graphic* graphic = g_iter.value();
        if (graphic && !graphic->IsInfinite())
            graphic->TranslateBy(reloc);
    }

    ListIter<SimLight> l_iter = scene->Lights();
    while (++l_iter)
    {
        SimLight* light = l_iter.value();
        if (light)
            light->TranslateBy(reloc);
    }

    camera->MoveTo(camera_loc);
}

// --------------------------------------------------------------------
// Visibility
// --------------------------------------------------------------------

void CameraView::MarkVisibleObjects()
{
    if (!Projector)
        return;

    Projector->StartFrame();
    graphics.clear();

    ListIter<Graphic> graphic_iter = scene->Graphics();
    while (++graphic_iter)
    {
        Graphic* graphic = graphic_iter.value();

        if (!graphic || graphic->Hidden())
            continue;

        if (graphic->CheckVisibility(*Projector))
        {
            graphic->Update();
            graphics.append(graphic);
        }
        else
        {
            graphic->ProjectScreenRect(0);
        }
    }
}

void CameraView::MarkVisibleLights(Graphic* graphic, uint32 flags)
{
    if (!graphic)
        return;

    if (flags < Graphic::RENDER_FIRST_LIGHT)
        flags = flags | Graphic::RENDER_FIRST_LIGHT | Graphic::RENDER_ADD_LIGHT;

    if (!graphic->IsVisible())
        return;

    ListIter<SimLight> light_iter = scene->Lights();
    while (++light_iter)
    {
        SimLight* light = light_iter.value();
        if (!light)
            continue;

        const LIGHTTYPE lt = static_cast<LIGHTTYPE>(light->Type());

        bool bright_enough =
            (lt == LIGHTTYPE::DIRECTIONAL) ||
            (light->Intensity() >= 1e9);

        if (!bright_enough)
        {
            const FVector test = graphic->Location() - light->Location();
            if (test.Size() < light->Intensity() * 10)
                bright_enough = true;
        }

        if (light->CastsShadow())
        {
            if ((flags & Graphic::RENDER_ADD_LIGHT) == 0)
                bright_enough = false;
        }
        else
        {
            if ((flags & Graphic::RENDER_FIRST_LIGHT) == 0)
                bright_enough = false;
        }

        light->SetActive(bright_enough);
    }
}

// --------------------------------------------------------------------
// Rendering passes (legacy Video pipeline preserved)
// --------------------------------------------------------------------

void CameraView::RenderBackground()
{
    if (scene->Background().isEmpty()) return;

    video->SetRenderState(Video::FILL_MODE, Video::FILL_SOLID);
    video->SetRenderState(Video::Z_ENABLE, FALSE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, FALSE);
    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::LIGHTING_ENABLE, TRUE);

    ListIter<Graphic> iter = scene->Background();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g && !g->Hidden())
            Render(g, Graphic::RENDER_SOLID);
    }

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g && !g->Hidden())
            Render(g, Graphic::RENDER_ALPHA);
    }

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g && !g->Hidden())
            Render(g, Graphic::RENDER_ADDITIVE);
    }
}

void CameraView::RenderForeground()
{
    bool foregroundVisible = false;

    ListIter<Graphic> iter = scene->Foreground();
    while (++iter && !foregroundVisible)
    {
        Graphic* g = iter.value();
        if (g && !g->Hidden())
            foregroundVisible = true;
    }

    if (!foregroundVisible)
        return;

    video->SetRenderState(Video::FILL_MODE, Video::FILL_SOLID);
    video->SetRenderState(Video::Z_ENABLE, TRUE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, TRUE);
    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::LIGHTING_ENABLE, TRUE);
    video->SetProjection((float)GetFieldOfView(), 1.0f, 1.0e6f, projection_type);

    if (video->IsShadowEnabled() || video->IsBumpMapEnabled())
    {
        iter.reset();
        while (++iter)
            Render(iter.value(), Graphic::RENDER_SOLID | Graphic::RENDER_FIRST_LIGHT);

        video->SetAmbient(FColor::Black);
        video->SetRenderState(Video::LIGHTING_PASS, 2);

        iter.reset();
        while (++iter)
            Render(iter.value(), Graphic::RENDER_SOLID | Graphic::RENDER_ADD_LIGHT);
    }
    else
    {
        iter.reset();
        while (++iter)
            Render(iter.value(), Graphic::RENDER_SOLID);
    }

    video->SetAmbient(scene->Ambient());
    video->SetRenderState(Video::LIGHTING_PASS, 0);
    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::Z_ENABLE, TRUE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, FALSE);

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g)
        {
            Render(g, Graphic::RENDER_ALPHA);
            g->ProjectScreenRect(Projector);
        }
    }

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g)
        {
            Render(g, Graphic::RENDER_ADDITIVE);
            g->ProjectScreenRect(Projector);
        }
    }
}

void CameraView::RenderSprites()
{
    if (scene->Sprites().isEmpty()) return;

    video->SetRenderState(Video::FILL_MODE, Video::FILL_SOLID);
    video->SetRenderState(Video::Z_ENABLE, TRUE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, FALSE);
    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::LIGHTING_ENABLE, TRUE);

    ListIter<Graphic> iter = scene->Sprites();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g && g->IsVisible() && !g->Hidden())
            FindDepth(g);
    }

    scene->Sprites().sort();

    iter.reset();
    while (++iter)
        Render(iter.value(), Graphic::RENDER_ALPHA);

    iter.reset();
    while (++iter)
        Render(iter.value(), Graphic::RENDER_ADDITIVE);
}

void CameraView::RenderScene()
{
    if (graphics.isEmpty()) return;

    ListIter<Graphic> iter = graphics;
    while (++iter)
    {
        Graphic* g = iter.value();
        if (g && !g->Hidden())
        {
            FindDepth(g);

            if (g->IsSolid())
            {
                Solid* solid = (Solid*)g;
                solid->SelectDetail(Projector);

                if (video->IsShadowEnabled())
                {
                    MarkVisibleLights(solid, Graphic::RENDER_ADD_LIGHT);
                    solid->UpdateShadows(scene->Lights());
                }
            }
        }
    }

    graphics.sort();

    Graphic* g = graphics.last();
    if (g && g->Depth() > 5e6)
    {
        RenderSceneObjects(true);
        video->ClearDepthBuffer();
    }

    RenderSceneObjects(false);
}

void CameraView::RenderSceneObjects(bool distant)
{
    ListIter<Graphic> iter = graphics;

    video->SetAmbient(scene->Ambient());
    video->SetRenderState(Video::FILL_MODE, Video::FILL_SOLID);
    video->SetRenderState(Video::Z_ENABLE, TRUE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, TRUE);
    video->SetRenderState(Video::LIGHTING_ENABLE, TRUE);

    if (distant)
        video->SetProjection((float)GetFieldOfView(), 5.0e6f, 1.0e12f, projection_type);
    else
        video->SetProjection((float)GetFieldOfView(), 1.0f, 1.0e6f, projection_type);

    if (video->IsShadowEnabled() || video->IsBumpMapEnabled())
    {
        iter.reset();
        while (++iter)
        {
            Graphic* g = iter.value();
            if (!g) continue;

            if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
                Render(g, Graphic::RENDER_SOLID | Graphic::RENDER_FIRST_LIGHT);
        }

        if (video->IsShadowEnabled())
        {
            iter.reset();
            while (++iter)
            {
                Graphic* g = iter.value();
                if (!g) continue;

                if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
                {
                    if (g->IsSolid())
                    {
                        Solid* solid = (Solid*)g;
                        ListIter<Shadow> shadow_iter = solid->GetShadows();
                        while (++shadow_iter)
                        {
                            Shadow* shadow = shadow_iter.value();
                            if (shadow)
                                shadow->Render(video);
                        }
                    }
                }
            }
        }

        video->SetAmbient(FColor::Black);
        video->SetRenderState(Video::LIGHTING_PASS, 2);
        video->SetRenderState(Video::STENCIL_ENABLE, TRUE);

        iter.reset();
        while (++iter)
        {
            Graphic* g = iter.value();
            if (!g) continue;

            if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
                Render(g, Graphic::RENDER_SOLID | Graphic::RENDER_ADD_LIGHT);
        }
    }
    else
    {
        iter.reset();
        while (++iter)
        {
            Graphic* g = iter.value();
            if (!g) continue;

            if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
                Render(g, Graphic::RENDER_SOLID);
        }
    }

    video->SetAmbient(scene->Ambient());
    video->SetRenderState(Video::LIGHTING_PASS, 0);
    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::Z_ENABLE, TRUE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, FALSE);

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (!g) continue;

        if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
        {
            Render(g, Graphic::RENDER_ALPHA);
            g->ProjectScreenRect(Projector);
        }
    }

    iter.reset();
    while (++iter)
    {
        Graphic* g = iter.value();
        if (!g) continue;

        if ((distant && g->Depth() > 5e6) || (!distant && g->Depth() < 5e6))
        {
            Render(g, Graphic::RENDER_ADDITIVE);
            g->ProjectScreenRect(Projector);
        }
    }
}

void CameraView::Render(Graphic* g, uint32 flags)
{
    if (g && g->IsVisible() && !g->Hidden())
    {
        if (g->IsSolid())
        {
            MarkVisibleLights(g, flags);
            video->SetLights(scene->Lights());
        }

        g->Render(video, flags);
    }
}

// --------------------------------------------------------------------
// Lens flare (draw through View::DrawBitmap)
// --------------------------------------------------------------------

void CameraView::RenderLensFlare()
{
    if (!lens_flare_enable || lens_flare_dim < 0.01)
        return;

    if (!halo_bitmap || !video || !scene || !camera || !Projector)
        return;

    video->SetRenderState(Video::STENCIL_ENABLE, FALSE);
    video->SetRenderState(Video::Z_ENABLE, FALSE);
    video->SetRenderState(Video::Z_WRITE_ENABLE, FALSE);

    const FVector center((float)width / 2.0f, (float)height / 2.0f, 1.0f);

    ListIter<SimLight> light_iter = scene->Lights();
    while (++light_iter)
    {
        SimLight* light = light_iter.value();
        if (!light || !light->IsActive())
            continue;

        const LIGHTTYPE lt = static_cast<LIGHTTYPE>(light->Type());

        if (lt == LIGHTTYPE::DIRECTIONAL && light->Intensity() < 1)
            continue;

        const double distance = (light->Location() - camera->Pos()).Size();

        if (distance > 1e9)
        {
            if (Projector->IsVisible(light->Location(), 1.0f))
            {
                FVector sun_pos = light->Location();

                if (light->CastsShadow() && scene->IsLightObscured(camera->Pos(), sun_pos, -1))
                    continue;

                Projector->Transform(sun_pos);

                if (sun_pos.Z < 100)
                    continue;

                Projector->Project(sun_pos, false);

                int x = (int)(sun_pos.X);
                int y = (int)(sun_pos.Y);
                int w = (int)(width / 4.0);
                int h = w;

                DrawBitmap(x - w, y - h, x + w, y + h, halo_bitmap, Video::BLEND_ADDITIVE);

                if (elem_bitmap[0])
                {
                    const FVector vec = center - sun_pos;
                    const float   vlen = vec.Size();
                    if (vlen < KINDA_SMALL_NUMBER)
                        continue;

                    const FVector dir = vec / vlen;

                    static int   nelem = 12;
                    static int   elem_indx[] = { 0,1,1,1,0,0,0,0,2,0,0,2 };
                    static float elem_dist[] = { -0.2f,0.5f,0.55f,0.62f,1.23f,1.33f,1.35f,0.8f,0.9f,1.4f,1.7f,1.8f };
                    static float elem_size[] = { 0.3f,0.2f,0.4f,0.3f,0.4f,0.2f,0.6f,0.1f,0.1f,1.6f,1.0f,0.2f };

                    for (int elem = 0; elem < nelem; elem++)
                    {
                        Bitmap* img = elem_bitmap[elem_indx[elem]];
                        if (!img) img = elem_bitmap[0];

                        const FVector flare_pos = sun_pos + (dir * elem_dist[elem] * vlen);

                        x = (int)(flare_pos.X);
                        y = (int)(flare_pos.Y);
                        w = (int)(width / 8.0f * elem_size[elem]);
                        h = w;

                        DrawBitmap(x - w, y - h, x + w, y + h, img, Video::BLEND_ADDITIVE);
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------------------

void CameraView::WorldPlaneToView(Plane& plane)
{
    const FVector tnormal = plane.normal;

    if (!infinite)
        plane.distance -= (float)(FVector::DotProduct(camera->Pos(), tnormal));

    plane.normal.X = (float)FVector::DotProduct(tnormal, cvrt);
    plane.normal.Y = (float)FVector::DotProduct(tnormal, cvup);
    plane.normal.Z = (float)FVector::DotProduct(tnormal, cvpn);
}

CameraView* CameraView::GetInstance()
{
    // Legacy pattern; you may want to manage this in a HUD/Screen manager later.
    if (!camera_view)
    {
        camera_view = new CameraView(nullptr, 0, 0, 0, 0, nullptr, nullptr);
    }
    return camera_view;
}
