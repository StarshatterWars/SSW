/*  Project Starshatter 4.5
    Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         HUDView.h
    AUTHOR:       John DiCamillo


    OVERVIEW
    ========
    View class for Heads Up Display
*/
#pragma once

#include "Math/Color.h"
#include "Math/Vector.h"
#include "View.h"
#include "GameStructs.h"
#include "Text.h"
#include "Sim.h"
#include "SimObject.h"
#include "Mouse.h"
#include "SimModel.h"

#include "CameraView.h"
#include "FontManager.h" 

#include "Layout/Geometry.h"
#include "Layout/SlateRect.h"
#include "Fonts/SlateFontInfo.h"
#include "Rendering/DrawElements.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"

// +--------------------------------------------------------------------+

class Graphic;
class Sprite;
class Solid;
class Ship;
class SimContact;
class Physical;
class OrbitalBody;
class OrbitalRegion;
class Instruction;
class SimProjector;

class Types;

class Bitmap;
class SystemFont;
class SimSystem;

// Views

class MFDView;


// +--------------------------------------------------------------------+

class HUDView : public View,
    public SimObserver
{
public:

    HUDView(Screen* InScreen);
    virtual ~HUDView();

   //using View::DrawRect;
    //using View::FillRect;
    //using View::DrawLine;
    using View::DrawPoly;
    //using View::DrawEllipse;

    // Operations:
    virtual void      Refresh();
    virtual void      OnWindowMove();
    virtual void      ExecFrame();
    virtual void      UseCameraView(CameraView* v);

    virtual Ship*       GetShip()      const { return ship; }
    virtual SimObject*  GetTarget()   const { return target; }
    virtual void        SetShip(Ship* s);
    virtual void        SetTarget(SimObject* t);
    virtual MFDView*    GetMFD(int n)  const;

    virtual void      HideAll();
    virtual void      DrawBars();
    virtual void      DrawNav();
    virtual void      DrawILS();
    virtual void      DrawObjective();
    virtual void      DrawNavInfo();
    virtual void      DrawNavPoint(Instruction& navpt, int index, int next);
    virtual void      DrawContactMarkers();
    virtual void      DrawContact(SimContact* c, int index);
    virtual void      DrawTrack(SimContact* c);
    virtual void      DrawTrackSegment(FVector& t1, FVector& t2, FColor c);
    virtual void      DrawRect(SimObject* targ);
    virtual void      DrawTarget();
    virtual void      DrawSight();
    virtual void      DrawLCOS(SimObject* targ, double dist);
    virtual void      DrawDesignators();
    virtual void      DrawFPM();
    virtual void      DrawHPM();
    virtual void      DrawCompass();
    virtual void      HideCompass();
    virtual void      DrawPitchLadder();
    virtual void      DrawStarSystem();

    virtual void      DrawMFDs();
    virtual void      DrawWarningPanel();
    virtual void      DrawInstructions();
    virtual void      DrawMessages();

    virtual void      MouseFrame();

    virtual EHUDMode  GetHUDMode()      const { return mode; }
    virtual int       GetTacticalMode() const { return tactical; }
    virtual void      SetTacticalMode(int mode = 1);
    virtual int       GetOverlayMode()  const { return overlay; }
    virtual void      SetOverlayMode(int mode = 1);

    virtual void      SetHUDMode(EHUDMode mode);
    virtual void      CycleHUDMode();
    virtual FColor    CycleHUDColor();
    virtual void      SetHUDColorSet(int c);
    virtual int       GetHUDColorSet()  const { return color; }
    virtual FColor    GetHUDColor()     const { return HudColor; }
    virtual FColor    GetTextColor()    const { return TextColor; }
    virtual FColor    Ambient()         const;
    virtual void      ShowHUDWarn();
    virtual void      ShowHUDInst();
    virtual void      HideHUDWarn();
    virtual void      HideHUDInst();
    virtual void      CycleHUDWarn();
    virtual void      CycleHUDInst();
    virtual void      CycleMFDMode(int mfd);
    virtual void      CycleInstructions(int direction);
    virtual void      RestoreHUD();

    virtual void      TargetOff() { target = 0; }
    static  FColor    MarkerColor(SimContact* targ);

    static bool       IsNameCrowded(int x, int y);
    static bool       IsMouseLatched();
    static HUDView*   GetInstance() { return hud_view; }
    static void       Message(const char* fmt, ...);
    static void       ClearMessages();
    static void       PrepareBitmap(const char* name, Bitmap& img, BYTE*& shades);
    static void       TransferBitmap(const Bitmap& src, Bitmap& img, BYTE*& shades);
    static void       ColorizeBitmap(Bitmap& img, BYTE* shades, FColor color, bool force_alpha = false);

    static int        GetGunsight() { return gunsight; }
    static void       SetGunsight(int s) { gunsight = s; }
    static bool       IsArcade() { return arcade; }
    static void       SetArcade(bool a) { arcade = a; }
    static int        DefaultColorSet() { return def_color_set; }
    static void       SetDefaultColorSet(int c) { def_color_set = c; }
    static FColor     GetStatusColor(SYSTEM_STATUS status);
    static bool       ShowFPS() { return show_fps; }
    static void       ShowFPS(bool f) { show_fps = f; }

    virtual bool         Update(SimObject* obj);
    virtual const char* GetObserverName() const;

    void DrawLine(int x1, int y1, int x2, int y2, FColor c);
    void FillRect(int x1, int y1, int x2, int y2, FColor c);
    void DrawRect(int x1, int y1, int x2, int y2, FColor c);
    void DrawEllipse(int x1, int y1, int x2, int y2, FColor c);
   
    // Legacy-style draw helpers:
    void DrawRect(const Rect& r, const FColor& c);
    void DrawRect(const Rect& r, const FLinearColor& c);
    // ----------------------------------------------------------------
    // Unreal/Slate paint binding (optional)
    // Call BeginPaint(...) from your widget's NativePaint,
    // then call HUDView::Refresh()/ExecFrame() as usual,
    // then EndPaint().
    // ----------------------------------------------------------------

public:
    // Bind a paint context for this frame (HUDView remains non-UObject).
    void BeginPaint(
        FPaintContext& InContext,
        const FGeometry& InGeometry,
        int32 InBaseLayer
    )
    {
        paint_ctx = &InContext;
        paint_geo = &InGeometry;
        paint_layer = InBaseLayer;
    }

    void EndPaint()
    {
        paint_ctx = nullptr;
        paint_geo = nullptr;
        // paint_layer intentionally left as-is
    }

protected:
    // Slate primitives (used by UE-backed DrawHUDText / DrawDiamond / etc.)
    void DrawLine(const FVector2D& A, const FVector2D& B, const FLinearColor& C, float Thickness) const;
    void DrawRectOutline(const FSlateRect& R, const FLinearColor& C, float Thickness) const;
    void FillRect(const FSlateRect& R, const FLinearColor& C) const;
    void DrawEllipseOutline(const FVector2D& Center, float Rx, float Ry, const FLinearColor& C, float Thickness, int32 Segments = 32) const;
    void DrawTextAt(const FVector2D& Pos, const FString& Text, const FSlateFontInfo& FontInfo, const FLinearColor& C) const;

    // Helpers: legacy->UE conversions
    static FLinearColor ToLinear(const Color& C);
    static FSlateRect   ToSlateRect(const Rect& R);

    // Active paint state (frame-scoped, not owned)
    FPaintContext* paint_ctx = nullptr;
    const FGeometry* paint_geo = nullptr;
    int32               paint_layer = 0;

    // Default HUD font info for Slate (override in .cpp if you have a real font asset)
    FSlateFontInfo      slate_hud_font;
    FSlateFontInfo      slate_big_font;

protected:
    const char*         FormatInstruction(Text instr);

    enum HUD_CASE { HUD_MIXED_CASE, HUD_UPPER_CASE };

    void              DrawDiamond(int x, int y, int r, FColor c);
    void              DrawHUDText(int index, const char* txt, Rect& rect, int align, int upcase = HUD_UPPER_CASE, bool box = false);
    void              HideHUDText(int index);

    void              DrawOrbitalBody(OrbitalBody* body);

    SimProjector* projector;
    CameraView* camview;

    int         width, height, aw, ah;
    double      xcenter, ycenter;

    Sim*        sim;
    Ship*       ship;
    SimObject*  target;

    SimRegion*  active_region;

    Bitmap*     cockpit_hud_texture;

    bool        show_warn;
    bool        show_inst;
    int         inst_page;
    int         threat;

    EHUDMode    mode;
    int         color;
    int         tactical;
    int         overlay;
    int         transition;
    int         docking;

    MFDView* mfd[3];

    Sprite*    pitch_ladder[31];
    Sprite*    hud_sprite[32];

    Solid*      az_ring;
    Solid*      az_pointer;
    Solid*      el_ring;
    Solid*      el_pointer;
    double      compass_scale;
    
    enum        { MAX_MSG = 6 };
    Text        msg_text[MAX_MSG];
    double      msg_time[MAX_MSG];

    static HUDView*   hud_view;
    static bool       arcade;
    static bool       show_fps;
    static int        gunsight;
    static int        def_color_set;
};

// +--------------------------------------------------------------------+

struct HUDText {
    SystemFont* font;
    FColor      color;
    Rect        rect;
    bool        hidden;

    FSlateFontInfo slate_font;
};

