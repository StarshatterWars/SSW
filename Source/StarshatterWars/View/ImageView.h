/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    nGenEx.lib
    FILE:         ImageView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    ImageView
    - Bitmap billboard renderer.
    - Centers image inside its parent View.
    - UE-port safe (no Window dependency in ctor).
*/

#pragma once

#include "Types.h"
#include "View.h"

// --------------------------------------------------------------------

class Bitmap;

// --------------------------------------------------------------------

class ImageView : public View
{
public:
    static const char* TYPENAME() { return "ImageView"; }

    // Construct as a child View (preferred in UE port)
    ImageView(View* InParent, Bitmap* InBitmap);
    virtual ~ImageView();

    // ------------------------------------------------------------
    // View interface
    // ------------------------------------------------------------
    virtual void Refresh() override;

    // ------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------
    Bitmap* GetPicture() const { return Image; }
    void    SetPicture(Bitmap* InBmp);

    int     GetBlend() const { return Blend; }
    void    SetBlend(int InBlend) { Blend = InBlend; }

protected:
    // ------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------
    Bitmap* Image = nullptr;

    // Cached image size (avoid collision with View::Width()/Height())
    int ImageW = 0;
    int ImageH = 0;

    // Draw offset relative to View origin
    int XOffset = 0;
    int YOffset = 0;

    int Blend = 0;
};
