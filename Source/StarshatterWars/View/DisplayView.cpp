/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC (1997-2004)

    SUBSYSTEM:    StarshatterWars
    FILE:         DisplayView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    DisplayView
    - Legacy HUD overlay message/image queue (ported).
    - Renders transient text/images with fade-in / hold / fade-out.
*/

#include "DisplayView.h"

#include "Game.h"         // Game::GUITime()
#include "Video.h"        // blend constants (Video::BLEND_*)
#include "View.h"       // Window size
#include "SystemFont.h"   // SystemFont wrapper
#include "Bitmap.h"       // if your Bitmap type is here; otherwise forward-only is fine

// Static instance:
DisplayView* DisplayView::display_view = nullptr;

DisplayView::DisplayView(Screen* InScreen, int ax, int ay, int aw, int ah)
    : View(InScreen, ax, ay, aw, ah)
{
    display_view = this;
    OnWindowMove();
}

DisplayView::~DisplayView()
{
    if (display_view == this)
        display_view = nullptr;

    elements.destroy();
}

DisplayView* DisplayView::GetInstance()
{
    // Legacy pattern; you may want to manage this in a HUD/Screen manager later.
    if (!display_view)
    {
        // Construct a safe dummy overlay. Caller should bind screen/window later.
        display_view = new DisplayView(nullptr, 0, 0, 0, 0);
    }
    return display_view;
}

void DisplayView::OnWindowMove()
{
    // Prefer actual window size (since View now owns Window*):
    if (GetWindow())
    {
        width = Width();
        height = Height();
    }
    else
    {
        // Fallback to View rect (useful if window not bound yet):
        width = Width();
        height = Height();
    }

    xcenter = (width / 2.0) - 0.5;
    ycenter = (height / 2.0) + 0.5;
}

void DisplayView::Refresh()
{
    ListIter<DisplayElement> iter = elements;
    while (++iter)
    {
        DisplayElement* elem = iter.value();
        if (!elem)
            continue;

        // Convert relative rect to window rect:
        Rect elem_rect = elem->rect;

        if (elem_rect.x == 0 && elem_rect.y == 0 && elem_rect.w == 0 && elem_rect.h == 0)
        {
            // Stretch to fit:
            elem_rect.w = width;
            elem_rect.h = height;
        }
        else if (elem_rect.w < 0 && elem_rect.h < 0)
        {
            // Center element in window:
            elem_rect.w *= -1;
            elem_rect.h *= -1;

            elem_rect.x = (width - elem_rect.w) / 2;
            elem_rect.y = (height - elem_rect.h) / 2;
        }
        else
        {
            // Offset from right/bottom:
            if (elem_rect.x < 0) elem_rect.x += width;
            if (elem_rect.y < 0) elem_rect.y += height;
        }

        // Compute current fade (legacy logic; assumes fades are ~1s or less):
        double fade = 0.0;
        if (elem->fade_in > 0)        fade = 1.0 - elem->fade_in;
        else if (elem->hold > 0)      fade = 1.0;
        else if (elem->fade_out > 0)  fade = elem->fade_out;

        if (fade < 0.0) fade = 0.0;
        if (fade > 1.0) fade = 1.0;

        // Draw text:
        if (elem->text.length() && elem->font)
        {
            elem->font->SetColor(elem->color);
            elem->font->SetAlpha(fade);

            SetFont(elem->font);
            this->DrawTextRect(elem->text.data(), elem->text.length(), elem_rect, DT_WORDBREAK);
        }

        // Draw image:
        else if (elem->image)
        {
            // Scale alpha by fade without changing RGB:
            FColor c = elem->color;
            const float a = (float)c.A / 255.0f;
            const float af = (float)(a * fade);
            c.A = (uint8)FMath::Clamp((int)(af * 255.0f), 0, 255);

            FadeBitmap(
                elem_rect.x,
                elem_rect.y,
                elem_rect.x + elem_rect.w,
                elem_rect.y + elem_rect.h,
                elem->image,
                c,
                elem->blend
            );
        }
    }
}

void DisplayView::ExecFrame()
{
    const double seconds = Game::GUITime();  // legacy GUI delta (or time-step)

    ListIter<DisplayElement> iter = elements;
    while (++iter)
    {
        DisplayElement* elem = iter.value();
        if (!elem)
            continue;

        if (elem->fade_in > 0)
            elem->fade_in -= seconds;

        else if (elem->hold > 0)
            elem->hold -= seconds;

        else if (elem->fade_out > 0)
            elem->fade_out -= seconds;

        else
            delete iter.removeItem();
    }
}

void DisplayView::ClearDisplay()
{
    elements.destroy();
}

void DisplayView::AddText(
    const char* txt,
    SystemFont* Font,
    const FColor& color,
    const Rect& text_rect,
    double          hold,
    double          fade_in,
    double          fade_out)
{
    DisplayElement* elem = new DisplayElement;

    if (fade_in == 0 && fade_out == 0 && hold == 0)
        hold = 300;

    elem->text = txt ? txt : "";
    elem->font = Font;
    elem->color = color;
    elem->rect = text_rect;
    elem->hold = hold;
    elem->fade_in = fade_in;
    elem->fade_out = fade_out;

    elements.append(elem);
}

void DisplayView::AddImage(
    Bitmap* bmp,
    const FColor& color,
    int             blend,
    const Rect& image_rect,
    double          hold,
    double          fade_in,
    double          fade_out)
{
    DisplayElement* elem = new DisplayElement;

    if (fade_in == 0 && fade_out == 0 && hold == 0)
        hold = 300;

    elem->image = bmp;
    elem->rect = image_rect;
    elem->color = color;
    elem->blend = blend;
    elem->hold = hold;
    elem->fade_in = fade_in;
    elem->fade_out = fade_out;

    elements.append(elem);
}
