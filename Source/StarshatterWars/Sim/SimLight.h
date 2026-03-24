/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         SimLight.h
    AUTHOR:       Carlos Bott
    ORIGINAL:     John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Dynamic Light Source (UE port)
    - Plain C++ class (not a UObject)
    - Uses FVector/FQuat/FColor
    - Adds Orientation() to support directional/spot light math (shadows, etc.)
*/

#pragma once

#include "Geometry.h"

// Minimal Unreal includes (required by request: convert Point/Vec3 to FVector):
#include "Math/Vector.h"
#include "Math/Quat.h"
#include "Math/Color.h"

#include "GameStructs.h"

// +--------------------------------------------------------------------+

#define SIMLIGHT_DESTROY(x) if (x) { x->Destroy(); x = 0; }

// +--------------------------------------------------------------------+
// Forward declarations (keep header light)

class UTexture2D;
class SimScene;

// +--------------------------------------------------------------------+

class SimLight
{
public:
    static const char* TYPENAME() { return "SimLight"; }

    SimLight(float l = 0.0f, float dl = 1.0f, int time = -1);
    virtual ~SimLight();

    int operator == (const SimLight& l) const { return id == l.id; }

    // --------------------------------------------------
    // operations
    // --------------------------------------------------
    virtual void Update();

    // --------------------------------------------------
    // identity
    // --------------------------------------------------
    int Identity() const { return id; }

    // --------------------------------------------------
    // transform
    // --------------------------------------------------
    FVector Location() const { return loc; }
    void    SetLocation(const FVector& p) { loc = p; }

    // NEW: world-space orientation (used for directional/spot math)
    FQuat   Orientation() const { return orient; }
    void    SetOrientation(const FQuat& q) { orient = q; }

    // NEW: world-space direction (forward vector from orientation)
    // Convention: forward vector indicates direction of emitted light rays.
    FVector Direction() const { return orient.GetForwardVector(); }

    // --------------------------------------------------
    // light properties
    // --------------------------------------------------
    LIGHTTYPE Type() const { return static_cast<LIGHTTYPE>(type); }
    void      SetType(LIGHTTYPE t) { type = t; }

    float     Intensity() const { return light; }
    void      SetIntensity(float f) { light = f; }

    FColor    GetColor() const { return color; }
    void      SetColor(FColor c) { color = c; }

    bool      IsActive() const { return active; }
    void      SetActive(bool a) { active = a; }

    bool      CastsShadow() const { return shadow; }
    void      SetShadow(bool s) { shadow = s; }

    bool      IsPoint() const { return type == LIGHTTYPE::POINT; }
    bool      IsSpot() const { return type == LIGHTTYPE::SPOT; }
    bool      IsDirectional() const { return type == LIGHTTYPE::DIRECTIONAL; }

    // --------------------------------------------------
    // movement
    // --------------------------------------------------
    virtual void MoveTo(const FVector& dst);
    virtual void TranslateBy(const FVector& ref);

    // --------------------------------------------------
    // lifetime
    // --------------------------------------------------
    virtual int  Life() const { return life; }
    virtual void Destroy();

    // --------------------------------------------------
    // scene binding
    // --------------------------------------------------
    virtual SimScene* GetScene() const { return scene; }
    virtual void      SetScene(SimScene* s) { scene = s; }

protected:
    static int  id_key;

    int         id;
    LIGHTTYPE   type;

    FVector     loc;

    // NEW: orientation in world space
    FQuat       orient;

    int         life;
    float       light;
    float       dldt;

    FColor      color;

    bool        active;
    bool        shadow;

    SimScene* scene;
};
