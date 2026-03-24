/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    nGenEx.lib
    FILE:         Polygon.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Polygon structures: VertexSet, Poly, Material
*/

#pragma once

#include "Geometry.h"
#include "Color.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class Bitmap;
struct Poly;
struct Material;
struct VertexSet;

// +--------------------------------------------------------------------+

struct Poly
{
    static const char* TYPENAME() { return "Poly"; }

    enum { MAX_VERTS = 4 };

    Poly() {}
    Poly(int init);
    ~Poly() {}

    int operator <  (const Poly& p) const { return sortval < p.sortval; }
    int operator == (const Poly& p) const { return this == &p; }

    int Contains(const FVector& pt) const;

    BYTE        nverts;
    BYTE        visible;
    WORD        verts[MAX_VERTS];
    WORD        vlocs[MAX_VERTS];
    VertexSet*  vertex_set;
    Material*   material;
    int         sortval;
    float       flatness;
    Plane       plane;
};

// +--------------------------------------------------------------------+

struct Material
{
    static const char* TYPENAME() { return "Material"; }

    enum BLEND_TYPE { MTL_SOLID = 1, MTL_TRANSLUCENT = 2, MTL_ADDITIVE = 4 };
    enum { NAMELEN = 32 };

    Material();
    ~Material();

    int operator == (const Material& m) const;

    void Clear();

    char        name[NAMELEN];
    char        shader[NAMELEN];

    FColor  Ka;         // ambient color was ColorValue)
    FColor  Kd;         // diffuse color
    FColor  Ks;         // specular color
    FColor  Ke;         // emissive color

    float       power;      // highlight sharpness (big=shiny)
    float       brilliance; // diffuse power function
    float       bump;       // bump level (0=none)
    DWORD       blend;      // alpha blend type
    bool        shadow;     // casts shadow
    bool        luminous;   // verts have their own lighting

    Bitmap* tex_diffuse;
    Bitmap* tex_specular;
    Bitmap* tex_bumpmap;
    Bitmap* tex_emissive;
    Bitmap* tex_alternate;
    Bitmap* tex_detail;

    bool        IsSolid()       const { return blend == MTL_SOLID; }
    bool        IsTranslucent() const { return blend == MTL_TRANSLUCENT; }
    bool        IsGlowing()     const { return blend == MTL_ADDITIVE; }
    const char* GetShader(int n) const;

    //
    // Support for Magic GUI
    //

    FColor      ambient_color;
    FColor      diffuse_color;
    FColor      specular_color;
    FColor      emissive_color;

    float       ambient_value;
    float       diffuse_value;
    float       specular_value;
    float       emissive_value;

    Bitmap* thumbnail;   // preview image

    void        CreateThumbnail(int size = 128);
    DWORD       GetThumbColor(int i, int j, int size);
};

// +--------------------------------------------------------------------+

struct VertexSet
{
    static const char* TYPENAME() { return "VertexSet"; }

    enum VertexSpaces { OBJECT_SPACE, WORLD_SPACE, VIEW_SPACE, SCREEN_SPACE };

    VertexSet(int m);
    ~VertexSet();

    void     Resize(int m, bool preserve = false);
    void     Delete();
    void     Clear();
    void     CreateTangents();
    void     CreateAdditionalTexCoords();
    bool     CopyVertex(int dst, int src);
    void     CalcExtents(FVector& plus, FVector& minus);

    VertexSet* Clone() const;

    int      nverts;
    int      space;

    FVector* loc;
    FVector* nrm;
    FVector* s_loc;
    float* rw;
    float* tu;
    float* tv;
    float* tu1;
    float* tv1;
    FColor* diffuse;
    FColor* specular;
    FVector* tangent;
    FVector* binormal;
};
