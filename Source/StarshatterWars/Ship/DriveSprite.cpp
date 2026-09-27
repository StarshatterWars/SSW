/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         DriveSprite.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Sprite for rendering drive flares. Remains visible at extreme ranges.
*/

#include "DriveSprite.h"

#include "Bitmap.h"
#include "Camera.h"
#include "SimScene.h"
#include "Video.h"

// Minimal Unreal includes (safe even if unused here; aligns with project convention):
#include "Logging/LogMacros.h"
#include "Math/Vector.h"   // FVector
#include "Math/Color.h"    // FColor

// +--------------------------------------------------------------------+

DriveSprite::DriveSprite()
    : Sprite(),
    effective_radius(0.0),
    front(FVector::ZeroVector),
    glow(nullptr),
    bias(0)
{
    luminous = true;
}

DriveSprite::DriveSprite(Bitmap* animation, Bitmap* g)
    : Sprite(animation),
    effective_radius(0.0),
    front(FVector::ZeroVector),
    glow(g),
    bias(0)
{
    luminous = true;
}

DriveSprite::DriveSprite(Bitmap* animation, int length, int repeat, int share)
    : Sprite(animation, length, repeat, share),
    effective_radius(0.0),
    front(FVector::ZeroVector),
    glow(nullptr),
    bias(0)
{
    luminous = true;
}

DriveSprite::~DriveSprite()
{
}

// +--------------------------------------------------------------------+

void
DriveSprite::SetFront(const FVector& f)
{
    front = f;
    front.Normalize();
}

void
DriveSprite::SetBias(DWORD b)
{
    bias = b;
}

// +--------------------------------------------------------------------+

void
DriveSprite::Render(Video* video, DWORD flags)
{
    if (!video || ((flags & RENDER_ADDITIVE) == 0))
        return;

    if (shade > 0 && !hidden && (life > 0 || loop)) {
        const Camera* cam = video->GetCamera();
        bool z_disable = false;

        if (bias)
            video->SetRenderState(Video::Z_BIAS, bias);

        if (!front.IsNearlyZero()) {
            const FVector Test = loc;

            if (scene && cam) {
                const FVector Dir = front;

                // Equivalent of: intensity = cam->vpn() * dir * -1
                const double Intensity = FVector::DotProduct(cam->vpn(), Dir) * -1.0;
                const double Distance = (cam->Pos() - Test).Size();

                if (Intensity > 0.05) {
                    if (!scene->IsLightObscured(cam->Pos(), Test, 8)) {
                        video->SetRenderState(Video::Z_ENABLE, false);
                        z_disable = true;

                        if (glow) {
                            double GlowIntensity = pow(Intensity, 3.0);

                            if (Distance > 5e3)
                                GlowIntensity *= (1.0 - (Distance - 5e3) / 45e3);

                            if (GlowIntensity > 0.0) {
                                Bitmap* TmpFrame = frames;
                                double  TmpShade = shade;
                                int     TmpW = w;
                                int     TmpH = h;

                                if (glow->Width() != frames->Width()) {
                                    const double WScale = (double)glow->Width() / (double)frames->Width();
                                    const double HScale = (double)glow->Height() / (double)frames->Height();

                                    w = (int)(w * WScale);
                                    h = (int)(h * HScale);
                                }

                                shade = GlowIntensity;
                                frames = glow;

                                Sprite::Render(video, flags);

                                frames = TmpFrame;
                                shade = TmpShade;
                                w = TmpW;
                                h = TmpH;
                            }
                        }
                    }
                }
            }
        }

        if (effective_radius - radius > 0.1) {
            const double ScaleUp = effective_radius / radius;
            const int TmpW = w;
            const int TmpH = h;

            w = (int)(w * ScaleUp);
            h = (int)(h * ScaleUp);

            Sprite::Render(video, flags);

            w = TmpW;
            h = TmpH;
        }
        else {
            Sprite::Render(video, flags);
        }

        if (bias)
            video->SetRenderState(Video::Z_BIAS, 0);

        if (z_disable)
            video->SetRenderState(Video::Z_ENABLE, true);
    }
}
