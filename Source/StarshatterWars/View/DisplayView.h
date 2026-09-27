/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC (1997-2004)

    SUBSYSTEM:    StarshatterWars
    FILE:         DisplayView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    DisplayView
    - Legacy HUD overlay message/image queue (ported).
    - Renders transient text/images with fade-in / hold / fade-out.
    - Uses the combined View (which owns Window* now).
*/

#pragma once

#include "Types.h"
#include "View.h"
#include "Geometry.h"
#include "Text.h"

#include "Math/Color.h" // FColor

class Bitmap;
class SystemFont;

class DisplayView : public View
{
public:
    static const char* TYPENAME() { return "DisplayView"; }

    // Root overlay (full-screen style). Uses View(Screen*, x,y,w,h).
    DisplayView(Screen* InScreen, int ax, int ay, int aw, int ah);
    virtual ~DisplayView();

    // View overrides:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;

    // Legacy-style tick:
    virtual void ExecFrame();
    virtual void ClearDisplay();

    // Add text/image elements:
    virtual void AddText(
        const char* txt,
        SystemFont* font,
        const FColor& color,
        const Rect& rect,
        double        hold = 1e9,
        double        fade_in = 0,
        double        fade_out = 0);

    virtual void AddImage(
        Bitmap* bmp,
        const FColor& color,
        int           blend,
        const Rect& rect,
        double        hold = 1e9,
        double        fade_in = 0,
        double        fade_out = 0);

    static DisplayView* GetInstance();

protected:
    int    width = 0;
    int    height = 0;
    double xcenter = 0.0;
    double ycenter = 0.0;

    struct DisplayElement
    {
        static const char* TYPENAME() { return "DisplayElement"; }

        Text        text;
        Bitmap* image = nullptr;
        SystemFont* font = nullptr;
        FColor      color = FColor::White;
        Rect        rect;
        int         blend = 0;
        double      hold = 0;
        double      fade_in = 0;
        double      fade_out = 0;
    };

    List<DisplayElement> elements;

private:
    static DisplayView* display_view;
};
