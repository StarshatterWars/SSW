/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025–2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         QuantumFlash.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Quantum Warp Out special effect class
*/

#include "QuantumFlash.h"

// --------------------------------------------------------------------
// Starshatter / engine includes
// --------------------------------------------------------------------

#include "Graphic.h"
#include "Polygon.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"
#include "Random.h"
#include "SimScene.h"
#include "Video.h"
#include "Camera.h"

// --------------------------------------------------------------------
// Minimal Unreal includes
// --------------------------------------------------------------------

#include "Math/Vector.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"
#include "Math/RandomStream.h"
#include "Logging/LogMacros.h"

// --------------------------------------------------------------------
// UE logging bridge (replaces Print / MemDebug)
// --------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogQuantumFlash, Log, All);

#ifndef Print
#define Print(...) UE_LOG(LogQuantumFlash, Log, TEXT(__VA_ARGS__))
#endif

// +--------------------------------------------------------------------+

static Bitmap* quantum_flash_texture = nullptr;

// +--------------------------------------------------------------------+

QuantumFlash::QuantumFlash()
    : length(8000.0)
    , width(0.0)
    , shade(1.0)
    , npolys(16)
    , nverts(64)
    , mtl(nullptr)
    , verts(nullptr)
    , polys(nullptr)
    , beams(nullptr)
    , texture(quantum_flash_texture)
{
    trans = true;
    luminous = true;

    // ------------------------------------------------------------
    // Ensure texture is loaded
    // ------------------------------------------------------------
    if (!texture || texture->Width() < 1)
    {
        DataLoader* loader = DataLoader::GetLoader();
        loader->SetDataPath("Explosions/");
        loader->LoadTexture("quantum.pcx",
            quantum_flash_texture,
            Bitmap::BMP_TRANSLUCENT);
        loader->SetDataPath(nullptr);

        texture = quantum_flash_texture;
    }

    // ------------------------------------------------------------
    // Initial location
    // ------------------------------------------------------------
    loc = FVector(0.0f, 0.0f, 1000.0f);

    // ------------------------------------------------------------
    // Allocate render data
    // ------------------------------------------------------------
    mtl = new Material;
    verts = new VertexSet(nverts);
    polys = new Poly[npolys];
    beams = new Matrix[npolys];

    // ------------------------------------------------------------
    // Material setup
    // ------------------------------------------------------------
    mtl->Kd = FColor::White;
    mtl->tex_diffuse = texture;
    mtl->tex_emissive = texture;
    mtl->blend = Video::BLEND_ADDITIVE;
    mtl->luminous = true;

    verts->nverts = nverts;

    // ------------------------------------------------------------
    // Build beam geometry
    // ------------------------------------------------------------
    for (int i = 0; i < npolys; ++i)
    {
        const int base = 4 * i;

        // Quad geometry (local space)
        verts->loc[base + 0] = FVector(width, 0.0f, 1000.0f);
        verts->loc[base + 1] = FVector(width, -length, 1000.0f);
        verts->loc[base + 2] = FVector(-width, -length, 1000.0f);
        verts->loc[base + 3] = FVector(-width, 0.0f, 1000.0f);

        for (int n = 0; n < 4; ++n)
        {
            const int idx = base + n;

            // FIX #1: correct color assignment (no uint32 packing)
            verts->diffuse[idx] = FColor::White;
            verts->specular[idx] = FColor::Black;

            verts->tu[idx] = (n < 2) ? 0.0f : 1.0f;
            verts->tv[idx] = (n > 0 && n < 3) ? 1.0f : 0.0f;
        }

        // FIX #2: no shadowed RandomStream, deterministic per beam
        FRandomStream BeamRng(i);

        beams[i].Roll(BeamRng.FRandRange(-2.0f * PI, 2.0f * PI));
        beams[i].Pitch(BeamRng.FRandRange(-2.0f * PI, 2.0f * PI));
        beams[i].Yaw(BeamRng.FRandRange(-2.0f * PI, 2.0f * PI));

        // Poly setup
        polys[i].nverts = 4;
        polys[i].visible = 1;
        polys[i].sortval = 0;
        polys[i].vertex_set = verts;
        polys[i].material = mtl;

        polys[i].verts[0] = base + 0;
        polys[i].verts[1] = base + 1;
        polys[i].verts[2] = base + 2;
        polys[i].verts[3] = base + 3;
    }

    // ------------------------------------------------------------
    // Finalize bounds
    // ------------------------------------------------------------
    radius = static_cast<float>(length);
    length = 0.0;

    strcpy_s(name, "QuantumFlash");
}

// +--------------------------------------------------------------------+

QuantumFlash::~QuantumFlash()
{
	delete mtl;
	delete verts;
	delete[] polys;
	delete[] beams;
}

// +--------------------------------------------------------------------+

void
QuantumFlash::Render(Video* video, DWORD flags)
{
	if (hidden || !visible || !video || ((flags & RENDER_ADDITIVE) == 0))
		return;

	const Camera* cam = video->GetCamera();
	if (cam) {
		UpdateVerts(cam->Pos());
		video->DrawPolys(npolys, polys);
	}
}

// +--------------------------------------------------------------------+

void QuantumFlash::UpdateVerts(const FVector& cam_pos)
{
    if (length < radius) {
        length += radius / 80.0;
        width += 1.0;
    }

    for (int i = 0; i < npolys; i++) {
        Matrix& m = beams[i];

        m.Yaw(0.05);

        const FVector vpn(m(2, 0), m(2, 1), m(2, 2));

        const FVector head = loc;
        const FVector tail = loc - vpn * length;
        const FVector vtail = tail - head;
        const FVector vcam = cam_pos - loc;

        FVector vtmp = FVector::CrossProduct(vcam, vtail);
        vtmp.Normalize();

        const FVector vlat = vtmp * -width;

        verts->loc[4 * i + 0] = head + vlat;
        verts->loc[4 * i + 1] = tail + vlat * 8.0f;
        verts->loc[4 * i + 2] = tail - vlat * 8.0f;
        verts->loc[4 * i + 3] = head - vlat;

        // OPTION A: write the actual color type (no packing)
        const uint8 c = (uint8)FMath::Clamp<int32>((int32)(255.0f * shade), 0, 255);
        const FColor color(c, c, c, 255);

        for (int n = 0; n < 4; n++) {
            verts->diffuse[4 * i + n] = color;
        }
    }
}

// +--------------------------------------------------------------------+

void
QuantumFlash::SetOrientation(const Matrix& o)
{
	orientation = o;
}

void
QuantumFlash::SetShade(double s)
{
	if (s < 0.0)      s = 0.0;
	else if (s > 1.0) s = 1.0;

	shade = s;
}
