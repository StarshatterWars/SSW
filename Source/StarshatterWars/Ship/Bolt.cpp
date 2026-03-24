/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         Bolt.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    3D Bolt (Polygon) Object
*/

#include "Bolt.h"

#include "Bitmap.h"
#include "Camera.h"
#include "Video.h"

// Minimal Unreal includes:
#include "Logging/LogMacros.h"
#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

Bolt::Bolt(double len, double wid, Bitmap* tex, int share)
    : vset(4),
    poly(0),
    texture(tex),
    shared(share),
    length(len),
    width(wid),
    shade(1.0),
    vpn(FVector(0.0f, 1.0f, 0.0f)),
    origin(FVector::ZeroVector)
{
    trans = true;

    // "loc" is a Starshatter core member (Vec3/Point originally).
    // In the Unreal port, the owning base types should have been migrated to FVector.
    loc = FVector(0.0f, 0.0f, 1000.0f);

    vset.nverts = 4;

    vset.loc[0] = FVector((float)width, 0.0f, 1000.0f);
    vset.loc[1] = FVector((float)width, (float)-length, 1000.0f);
    vset.loc[2] = FVector((float)-width, (float)-length, 1000.0f);
    vset.loc[3] = FVector((float)-width, 0.0f, 1000.0f);

    vset.tu[0] = 0.0f;
    vset.tv[0] = 0.0f;
    vset.tu[1] = 1.0f;
    vset.tv[1] = 0.0f;
    vset.tu[2] = 1.0f;
    vset.tv[2] = 1.0f;
    vset.tu[3] = 0.0f;
    vset.tv[3] = 1.0f;

    Plane plane(vset.loc[0], vset.loc[1], vset.loc[2]);

    for (int i = 0; i < 4; i++) {
        vset.nrm[i] = plane.normal;
    }

    mtl.Ka = FColor::White;
    mtl.Kd = FColor::White;
    mtl.Ks = FColor::Black;
    mtl.Ke = FColor::White;

    mtl.tex_diffuse = texture;
    mtl.tex_emissive = texture;
    mtl.blend = Video::BLEND_ADDITIVE;

    poly.nverts = 4;
    poly.vertex_set = &vset;
    poly.material = &mtl;
    poly.verts[0] = 0;
    poly.verts[1] = 1;
    poly.verts[2] = 2;
    poly.verts[3] = 3;

    radius = (float)((length > width) ? (length) : (width * 2));

    if (texture) {
        strncpy_s(name, texture->GetFilename(), 31);
        name[31] = 0;
    }
}

// +--------------------------------------------------------------------+

Bolt::~Bolt()
{
}

// +--------------------------------------------------------------------+

void
Bolt::Render(Video* video, DWORD flags)
{
    if ((flags & RENDER_ADDITIVE) == 0)
        return;

    if (visible && !hidden && video && life) {
        const Camera* camera = video->GetCamera();

        const FVector Head = loc;
        const FVector Tail = origin;

        const FVector VTail = Tail - Head;
        const FVector VCam = camera->Pos() - loc;

        // Equivalent of Point::cross + Normalize:
        FVector VTmp = FVector::CrossProduct(VCam, VTail);
        VTmp.Normalize();

        const FVector VLat = VTmp * (float)(-width);

        // Camera normal:
        const FVector Vnrm = camera->vpn() * -1.0f;

        vset.loc[0] = Head + VLat;
        vset.loc[1] = Tail + VLat;
        vset.loc[2] = Tail - VLat;
        vset.loc[3] = Head - VLat;

        vset.nrm[0] = Vnrm;
        vset.nrm[1] = Vnrm;
        vset.nrm[2] = Vnrm;
        vset.nrm[3] = Vnrm;

        // Starshatter ColorValue -> use Unreal FColor (grayscale shade).
        // Clamp shade to [0..1], then map to [0..255].
        double ShadeClamped = shade;
        if (ShadeClamped < 0.0) ShadeClamped = 0.0;
        if (ShadeClamped > 1.0) ShadeClamped = 1.0;

        const uint8 ShadeByte = (uint8)(ShadeClamped * 255.0);

        const FColor White(ShadeByte, ShadeByte, ShadeByte, 255);

        mtl.Ka = White;
        mtl.Kd = White;
        mtl.Ks = FColor::Black;
        mtl.Ke = White;

        video->DrawPolys(1, &poly);
    }
}

// +--------------------------------------------------------------------+

void
Bolt::Update()
{
}

// +--------------------------------------------------------------------+

void
Bolt::TranslateBy(const FVector& Ref)
{
    loc = loc - Ref;
    origin = origin - Ref;
}

// +--------------------------------------------------------------------+

void
Bolt::SetOrientation(const Matrix& o)
{
    // Matrix accessor o(r,c) expected to remain Starshatter-style.
    vpn = FVector((float)o(2, 0), (float)o(2, 1), (float)o(2, 2));
    origin = loc + (vpn * (float)-length);
}

void
Bolt::SetDirection(const FVector& v)
{
    vpn = v;
    origin = loc + (vpn * (float)-length);
}

void
Bolt::SetEndPoints(const FVector& From, const FVector& To)
{
    loc = To;
    origin = From;

    vpn = To - From;

    // Starshatter Normalize() returned length; emulate that:
    length = (double)vpn.Size();
    if (length > 0.0)
        vpn /= (float)length;

    radius = (float)length;
}

void
Bolt::SetTextureOffset(double from, double to)
{
    vset.tu[0] = (float)from;
    vset.tu[1] = (float)to;
    vset.tu[2] = (float)to;
    vset.tu[3] = (float)from;
}
