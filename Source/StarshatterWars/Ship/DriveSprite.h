/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         DriveSprite.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Specialized Drive Sprite Object
*/

#pragma once

#include "Types.h"
#include "Sprite.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+
// Forward Declarations
// +--------------------------------------------------------------------+

class Bitmap;
class Video;

// +--------------------------------------------------------------------+

class DriveSprite : public Sprite
{
public:
    DriveSprite();
    DriveSprite(Bitmap* animation, Bitmap* glow);
    DriveSprite(Bitmap* animation, int length = 1, int repeat = 1, int share = 1);
    virtual ~DriveSprite();

    // operations
    virtual void   Render(Video* video, DWORD flags);
    virtual void   SetFront(const FVector& f);
    virtual void   SetBias(DWORD b);

protected:
    double         effective_radius = 0.0;
    FVector        front = FVector::ZeroVector;
    Bitmap* glow = nullptr;
    DWORD          bias = 0;
};
