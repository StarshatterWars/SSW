/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipSolid.cpp
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
*/

#include "ShipSolid.h"

#include "Ship.h"
#include "Sim.h"
#include "StarSystem.h"
#include "TerrainRegion.h"
#include "Game.h"
#include "Skin.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

ShipSolid::ShipSolid(Ship* s)
    : ship(s), skin(nullptr), in_soup(false)
{
}

// +--------------------------------------------------------------------+

ShipSolid::~ShipSolid()
{
}

// +--------------------------------------------------------------------+

void
ShipSolid::TranslateBy(const FVector& ref)
{
    true_eye_point = ref;

    // NOTE:
    // Solid::TranslateBy historically took a Starshatter Point.
    // In the Unreal port, Solid should expose an overload that accepts FVector,
    // or provide a conversion helper (e.g., ToPoint/ToFVector).
    Solid::TranslateBy(ref);
}

// +--------------------------------------------------------------------+

void
ShipSolid::Render(Video* video, DWORD flags)
{
    if (hidden || !visible || !video || Depth() > 5e6)
        return;

    const Skin* s = nullptr;

    if (ship)
        s = ship->GetSkin();
    else
        s = skin;

    if (s)
        s->ApplyTo(model);

    bool fog = false;

    if (ship && ship->IsAirborne()) {
        fog = true;

        TerrainRegion* rgn = (TerrainRegion*)ship->GetRegion()->GetOrbitalRegion();
        const double   visibility = rgn->GetWeather().Visibility();
        const float    fogDensity = (float)(rgn->FogDensity() * 2.5e-5 * (1.0 / visibility));
        FColor         fogColor = rgn->FogColor();

        // Use BLACK fog on secondary lighting pass.
        // This effectively "filters out" highlights with distance.
        if (flags & Graphic::RENDER_ADD_LIGHT)
            fogColor = FColor::Black;

        video->SetRenderState(Video::FOG_ENABLE, true);
       
       const DWORD FogColorValue =
            (DWORD(fogColor.A) << 24) |
            (DWORD(fogColor.R) << 16) |
            (DWORD(fogColor.G) << 8) |
            (DWORD(fogColor.B));

        video->SetRenderState(Video::FOG_COLOR, FogColorValue);

        // Preserve legacy expectation: engine takes raw bits for density.
        const DWORD fogBits = *reinterpret_cast<const DWORD*>(&fogDensity);
        video->SetRenderState(Video::FOG_DENSITY, fogBits);
    }

    if (!fog)
        video->SetRenderState(Video::FOG_ENABLE, false);

    Solid::Render(video, flags);

    if (fog)
        video->SetRenderState(Video::FOG_ENABLE, false);

    if (s)
        s->Restore(model);
}
