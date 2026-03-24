/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainClouds.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	Terrain cloud bank graphic (high/low cloud layers rendered above terrain).
*/

#include "TerrainClouds.h"
#include "Terrain.h"
#include "TerrainRegion.h"

#include "SimLight.h"
#include "CameraView.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"
#include "Fix.h"
#include "SimScene.h"

// Unreal minimal includes:
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

// +--------------------------------------------------------------------+

TerrainClouds::TerrainClouds(Terrain* terr, int t)
	: terrain(terr), type(t)
{
	nverts = 0;
	npolys = 0;
	mverts = nullptr;
	verts = nullptr;
	polys = nullptr;

	loc = FVector(0.0, 15000.0, 0.0);
	radius = 25000.0f;

	BuildClouds();
}

// +--------------------------------------------------------------------+

TerrainClouds::~TerrainClouds()
{
	delete[] mverts;
	delete   verts;
	delete[] polys;
}

// +--------------------------------------------------------------------+

static const double BANK_SIZE = 20000;
static const int    CIRRUS_BANKS = 4;
static const int    CUMULUS_BANKS = 4;

void
TerrainClouds::BuildClouds()
{
	if (type == 0) {
		nbanks = CIRRUS_BANKS;
		nverts = 4 * nbanks;
		npolys = 2 * nbanks;
	}
	else {
		nbanks = CUMULUS_BANKS;
		nverts = 8 * nbanks;
		npolys = 3 * nbanks;
	}

	Bitmap* cloud_texture = terrain ? terrain->CloudTexture(type) : nullptr;
	Bitmap* shade_texture = terrain ? terrain->ShadeTexture(type) : nullptr;

	strcpy_s(mtl_cloud.name, "cloud");
	mtl_cloud.Ka = FColor::White;
	mtl_cloud.Kd = FColor::White;
	mtl_cloud.luminous = true;
	mtl_cloud.blend = Material::MTL_TRANSLUCENT;
	mtl_cloud.tex_diffuse = cloud_texture;

	strcpy_s(mtl_shade.name, "shade");
	mtl_shade.Ka = FColor::White;
	mtl_shade.Kd = FColor::White;
	mtl_shade.luminous = true;
	mtl_shade.blend = Material::MTL_TRANSLUCENT;
	mtl_shade.tex_diffuse = shade_texture;

	verts = new VertexSet(nverts);
	mverts = new FVector[nverts];
	polys = new Poly[npolys];

	verts->nverts = nverts;

	// initialize vertices:
	FVector* pVert = mverts;
	float* pTu = verts->tu;
	float* pTv = verts->tv;

	int    i = 0, j = 0, n = 0;
	double az = 0;
	double r = 0;

	for (n = 0; n < nbanks; n++) {
		const double xloc = r * cos(az);
		const double yloc = r * sin(az);

		// rand()/32.768 -> 0..~1000; use UE random:
		const double alt = (double)FMath::FRandRange(0.0f, 1000.0f);

		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
				*pVert = FVector(
					(double)((2 * j - 1) * BANK_SIZE + xloc),
					(double)alt,
					(double)((2 * i - 1) * BANK_SIZE + yloc)
				);

				*pTu++ = (float)(-j);
				*pTv++ = (float)(i);

				const float dist = (float)pVert->Length();
				if (dist > radius)
					radius = dist;

				++pVert;
			}
		}

		if (type > 0) {
			for (i = 0; i < 2; i++) {
				for (j = 0; j < 2; j++) {
					*pVert = FVector(
						(double)((2 * j - 1) * BANK_SIZE + xloc),
						(double)(alt - 100.0),
						(double)((2 * i - 1) * BANK_SIZE + yloc)
					);

					*pTu++ = (float)(-j);
					*pTv++ = (float)(i);

					const float dist = (float)pVert->Length();
					if (dist > radius)
						radius = dist;

					++pVert;
				}
			}
		}

		// az += (0.66 + rand()/32768.0) * 0.25 * PI;
		az += (0.66 + (double)FMath::FRandRange(0.0f, 1.0f)) * 0.25 * PI;

		if (r < BANK_SIZE)
			r += BANK_SIZE;
		else if (r < 1.75 * BANK_SIZE)
			r += BANK_SIZE / 4;
		else
			r += BANK_SIZE / 8;
	}

	// create the polys:
	for (i = 0; i < npolys; i++) {
		Poly* p = polys + i;
		p->nverts = 4;
		p->vertex_set = verts;
		p->material = (i < 4 * nbanks) ? &mtl_cloud : &mtl_shade;
		p->visible = 1;
		p->sortval = (i < 4 * nbanks) ? 1 : 2;
	}

	// build main patch polys: (facing down)
	Poly* p = polys;

	int stride = (type > 0) ? 8 : 4;

	// clouds:
	for (n = 0; n < nbanks; n++) {
		p->verts[0] = 0 + n * stride;
		p->verts[1] = 1 + n * stride;
		p->verts[2] = 3 + n * stride;
		p->verts[3] = 2 + n * stride;
		++p;

		// reverse side: (facing up)
		p->verts[0] = 0 + n * stride;
		p->verts[3] = 1 + n * stride;
		p->verts[2] = 3 + n * stride;
		p->verts[1] = 2 + n * stride;
		++p;
	}

	// shades:
	if (type > 0) {
		for (n = 0; n < nbanks; n++) {
			p->verts[0] = 4 + n * stride;
			p->verts[1] = 5 + n * stride;
			p->verts[2] = 7 + n * stride;
			p->verts[3] = 6 + n * stride;
			++p;
		}
	}

	// update planes:
	for (i = 0; i < npolys; i++) {
		Poly* ppoly = polys + i;
		WORD* v = ppoly->verts;

		// Plane expects the engine's vector type used in Geometry.h.
		// If Plane is still Starshatter-core (not UE), ensure it has an overload
		// accepting FVector or provide a conversion helper there.
		ppoly->plane = Plane(mverts[v[0]], mverts[v[1]], mverts[v[2]]);
	}
}

// +--------------------------------------------------------------------+

void
TerrainClouds::Update()
{
	if (!nverts || !mverts || !verts)
		return;

	for (int i = 0; i < nverts; ++i)
		verts->loc[i] = mverts[i] + loc;
}

// +--------------------------------------------------------------------+

void
TerrainClouds::Illuminate(FColor Ambient, List<SimLight>& Lights)
{
	(void)Ambient;
	(void)Lights;

	if (!terrain || !verts || !terrain->GetRegion() || !verts->diffuse || !verts->specular)
		return;

	const FColor CloudColor = terrain->GetRegion()->CloudColor().WithAlpha(255);
	const FColor ShadeColor = terrain->GetRegion()->ShadeColor().WithAlpha(255);

	const int32 Stride = (type > 0) ? 8 : 4;

	// Starshatter behavior: clouds are pre-colored per bank, not dynamically lit.
	for (int32 BankIndex = 0; BankIndex < nbanks; ++BankIndex) {

		// Primary cloud layer
		for (int32 VertexInBank = 0; VertexInBank < 4; ++VertexInBank) {
			const int32 Index = Stride * BankIndex + VertexInBank;
			verts->diffuse[Index] = CloudColor;
			verts->specular[Index] = FColor(0, 0, 0, 255); // opaque black specular
		}

		// Secondary shade layer (only for layered clouds)
		if (type > 0) {
			for (int32 VertexInBank = 4; VertexInBank < 8; ++VertexInBank) {
				const int32 Index = Stride * BankIndex + VertexInBank;
				verts->diffuse[Index] = ShadeColor;
				verts->specular[Index] = FColor(0, 0, 0, 255);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void
TerrainClouds::Render(Video* video, DWORD flags)
{
	if ((flags & Graphic::RENDER_ALPHA) == 0)
		return;

	if (video && life && polys && npolys && verts) {
		if (scene) {
			// scene->Lights() must be List<SimLight> per project mapping.
			Illuminate(scene->Ambient(), scene->Lights());
		}

		video->SetRenderState(Video::FOG_ENABLE, false);
		video->DrawPolys(npolys, polys);
	}
}
