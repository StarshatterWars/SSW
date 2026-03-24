/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    SUBSYSTEM:    nGenEx.lib (ported to Unreal)
    FILE:         CameraView.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC
    Copyright (C) 1997–2004.

    OVERVIEW
    ========
    CameraView
    ----------
    Legacy 3D projection camera view.

    CameraView is a non-UObject view that configures the Video/Projector
    pipeline to render a Scene from a given Camera into the view rectangle.
    Rendering remains delegated to the Video layer; this class coordinates
    visibility, sorting, projection, and optional lens flare.
*/

#pragma once

// Macro collision defense:
#ifdef projector
#undef projector
#endif

#ifdef Projector
#undef Projector
#endif

#include "View.h"
#include "Types.h"
#include "List.h"
#include "Geometry.h"     // Rect, Plane (legacy)

#include "Math/Vector.h"  // FVector

// Forward declarations (keep header light):
class Screen;
class Camera;
class SimScene;
class SimLight;
class SimProjector;
class Video;
class Bitmap;
class Graphic;
class Solid;
class Shadow;

// If your Matrix type is legacy, forward declare it:
struct Matrix;

// LIGHTTYPE enum is in GameStructs.h:
#include "GameStructs.h"

class CameraView : public View
{
public:
    static const char* TYPENAME() { return "CameraView"; }

    CameraView(Screen* InScreen, int ax, int ay, int aw, int ah, Camera* cam, SimScene* s);
    virtual ~CameraView() override;

    // View lifecycle:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;

    // Operations:
    virtual void   UseCamera(Camera* cam);
    virtual void   UseScene(SimScene* scene);

    // Lens flare:
    virtual void   LensFlareElements(Bitmap* halo, Bitmap* e1 = nullptr, Bitmap* e2 = nullptr, Bitmap* e3 = nullptr);
    virtual void   LensFlare(int on, double dim = 1.0);
    virtual void   RenderLensFlare();

    // Projector / projection:
    virtual void   SetFieldOfView(double fov);
    virtual double GetFieldOfView() const;

    virtual void   SetProjectionType(uint32 pt);
    virtual uint32 GetProjectionType() const;

    virtual void   SetDepthScale(float scale);
    virtual int    SetInfinite(int i);

    // Accessors:
    Camera* GetCamera() const { return camera; }
    SimScene* GetScene()  const { return scene; }

    SimProjector* GetProjector() { return Projector; }
    const SimProjector* GetProjector() const { return Projector; }

    // Convenience pass-throughs (legacy naming):
    FVector        Pos() const;
    FVector        vrt() const;
    FVector        vup() const;
    FVector        vpn() const;

    const Matrix& Orientation() const;

    FVector        SceneOffset() const { return camera_loc; }

    // Pipeline stages:
    virtual void   TranslateScene();
    virtual void   UnTranslateScene();
    virtual void   MarkVisibleObjects();
    virtual void   MarkVisibleLights(Graphic* g, uint32 flags);

    virtual void   RenderScene();
    virtual void   RenderSceneObjects(bool distant = false);
    virtual void   RenderForeground();
    virtual void   RenderBackground();
    virtual void   RenderSprites();
    virtual void   Render(Graphic* g, uint32 flags);

    virtual void   FindDepth(Graphic* g);

    static CameraView* GetInstance();

protected:
    virtual void   WorldPlaneToView(Plane& plane);

protected:
    Camera* camera = nullptr;
    SimScene* scene = nullptr;
    Video* video = nullptr;

    // New UE vectors:
    FVector         camera_loc = FVector::ZeroVector;
    FVector         cvrt = FVector::ZeroVector;
    FVector         cvup = FVector::ZeroVector;
    FVector         cvpn = FVector::ZeroVector;

    // Projector owned here:
    SimProjector* Projector = nullptr;

    int             infinite = 0;
    int             width = 0;
    int             height = 0;
    uint32          projection_type = 0;

    // Lens flare:
    int             lens_flare_enable = 0;
    double          lens_flare_dim = 0.0;
    Bitmap* halo_bitmap = nullptr;
    Bitmap* elem_bitmap[3] = { nullptr, nullptr, nullptr };

    // Visible scene list:
    List<Graphic>   graphics;

private:
    static CameraView* camera_view;
};
