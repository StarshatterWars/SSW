/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         ShipSolid.h
    AUTHOR:       Carlos Bott


    OVERVIEW
    ========
    3D Solid (Polygon) Object
*/

#pragma once

#include "Solid.h"

// +--------------------------------------------------------------------+

class Ship;
class Skin;

// +--------------------------------------------------------------------+

class ShipSolid : public Solid
{
public:
    static const char* TYPENAME() { return "ShipSolid"; }

    ShipSolid(Ship* s);
    virtual ~ShipSolid();

    virtual void   Render(Video* video, DWORD flags) override;
    virtual void   TranslateBy(const FVector& ref) override;

    const Skin* GetSkin() const { return skin; }
    void           SetSkin(const Skin* s) { skin = s; }

protected:
    Ship* ship = nullptr;
    const Skin* skin = nullptr;

    FVector        true_eye_point = FVector::ZeroVector;
    FVector        fog_loc = FVector::ZeroVector;

    bool           in_soup = false;
};
