/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainApron.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
*/

#include "TerrainApron.h"
#include "GameStructs.h"

#include "Terrain.h"
#include "TerrainRegion.h"

#include "CameraView.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Game.h"
#include "SimLight.h"
#include "SimModel.h"
#include "Segment.h"
#include "Surface.h"
#include "SimScene.h"
#include "GameStructs.h"

#include "Math/UnrealMathUtility.h" // FMath
#include "HAL/UnrealMemory.h" 

// +====================================================================+



TerrainApron::TerrainApron(
	Terrain* terr,
	const Bitmap* patch,
	const Rect& r,
	const FVector& p1,
	const FVector& p2
)
	: terrain(terr), rect(r)
{
	size = FMath::Abs(p2.X - p1.X);
	scale = size / (PATCH_SIZE - 1);
	mtnscale = 1.3 * (p2.Y - p1.Y);
	base = p1.Y;

	terrain_width = patch->Width();

	loc = (p1 + p2) * 0.5f;
	loc.Y = (float)base;

	radius = (float)(size * 0.75);
	heights = (float*)FMemory::Malloc(sizeof(float) * MAX_VERTS);

	float* pHeight = heights;

	int i, j;

	for (i = 0; i < PATCH_SIZE; i++) {
		int ty = rect.y + i;

		if (ty < 0)
			ty = 0;

		if (ty > patch->Height() - 1)
			ty = patch->Height() - 1;

		for (j = 0; j < PATCH_SIZE; j++) {
			int tx = rect.x + (PATCH_SIZE - 1 - j);

			if (tx < 0)
				tx = 0;

			if (tx > patch->Width() - 1)
				tx = patch->Width() - 1;

			*pHeight++ = (float)(patch->GetColor(tx, ty).R * mtnscale);
		}
	}
}

// +--------------------------------------------------------------------+

TerrainApron::~TerrainApron()
{
	if (heights) {
		FMemory::Free(heights);
		heights = nullptr;
	}
}

// +--------------------------------------------------------------------+

void
TerrainApron::SetScales(double s, double m, double b)
{
	scale = s;
	mtnscale = m;
	base = b;
}

// +--------------------------------------------------------------------+

bool
TerrainApron::BuildApron()
{
	// UE-style internal variable types:
	int32 i = 0;
	int32 j = 0;

	const int32 DetailSize = PATCH_SIZE - 1;
	const int32 Ds1 = PATCH_SIZE;
	const int32 NumVerts = MAX_VERTS;
	const int32 NumPolys = DetailSize * DetailSize * 2;

	model = new SimModel;
	model->SetLuminous(true);
	model->SetDynamic(true);

	Material* mtl = new Material;

	mtl->Ka = FColor(
		(uint8)(0.5f * 255.0f),
		(uint8)(0.5f * 255.0f),
		(uint8)(0.5f * 255.0f),
		255
	);

	mtl->Kd = FColor(
		(uint8)(0.3f * 255.0f),
		(uint8)(0.6f * 255.0f),
		(uint8)(0.2f * 255.0f),
		255
	);

	mtl->Ks = FColor::Black;

	mtl->tex_diffuse = terrain->ApronTexture();
	strcpy_s(mtl->name, "Terrain Apron");

	model->GetMaterials().append(mtl);

	Surface* s = new Surface;
	VertexSet* vset = nullptr;

	if (s) {
		s->SetName("default");
		s->CreateVerts(NumVerts);
		s->CreatePolys(NumPolys);
		s->AddIndices(NumPolys * NUM_INDICES_TRI);

		vset = s->GetVertexSet();

		FMemory::Memzero(vset->loc, NumVerts * sizeof(FVector));
		FMemory::Memzero(vset->diffuse, NumVerts * sizeof(DWORD));
		FMemory::Memzero(vset->specular, NumVerts * sizeof(DWORD));
		FMemory::Memzero(vset->tu, NumVerts * sizeof(float));
		FMemory::Memzero(vset->tv, NumVerts * sizeof(float));
		FMemory::Memzero(vset->rw, NumVerts * sizeof(float));

		// initialize vertices
		FVector* pVert = vset->loc;
		float* pTu = vset->tu;
		float* pTv = vset->tv;

		const double Dt = (1.0 / 3.0) / (double)DetailSize;
		const double Tu0 = (double)rect.x / rect.w / 3.0 + (1.0 / 3.0);
		const double Tv0 = (double)rect.y / rect.h / 3.0;

		for (i = 0; i < Ds1; i++) {
			for (j = 0; j < Ds1; j++) {
				*pVert++ = FVector(
					(float)(j * scale - (HALF_PATCH_SIZE * scale)),
					(float)(heights[i * PATCH_SIZE + j]),
					(float)(i * scale - (HALF_PATCH_SIZE * scale))
				);

				*pTu++ = (float)(Tu0 - j * Dt);
				*pTv++ = (float)(Tv0 + i * Dt);
			}
		}

		// create the polys
		for (i = 0; i < NumPolys; i++) {
			Poly* p = s->GetPolys() + i;
			p->nverts = 3;
			p->vertex_set = vset;
			p->material = mtl;
			p->visible = 1;
			p->sortval = 0;
		}

		int32 Index = 0;

		// build main patch polys:
		for (i = 0; i < DetailSize; i++) {
			for (j = 0; j < DetailSize; j++) {
				// first triangle
				Poly* p = s->GetPolys() + Index++;
				p->verts[0] = (Ds1 * (i)+(j));
				p->verts[1] = (Ds1 * (i)+(j + 1));
				p->verts[2] = (Ds1 * (i + 1) + (j + 1));

				// second triangle
				p = s->GetPolys() + Index++;
				p->verts[0] = (Ds1 * (i)+(j));
				p->verts[1] = (Ds1 * (i + 1) + (j + 1));
				p->verts[2] = (Ds1 * (i + 1) + (j));
			}
		}

		// update the verts and colors of each poly:
		for (i = 0; i < NumPolys; i++) {
			Poly* p = s->GetPolys() + i;
			Plane& plane = p->plane;
			WORD* v = p->verts;

			plane = Plane(
				vset->loc[v[0]] + loc,
				vset->loc[v[1]] + loc,
				vset->loc[v[2]] + loc
			);
		}

		// create contiguous segments for each material:
		s->Normalize();

		Segment* segment = new Segment(NumPolys, s->GetPolys(), mtl, model);
		s->GetSegments().append(segment);

		model->AddSurface(s);

		// copy vertex normals:
		const FVector Normal(0.0f, 1.0f, 0.0f);

		for (i = 0; i < Ds1; i++) {
			for (j = 0; j < Ds1; j++) {
				vset->nrm[i * Ds1 + j] = Normal;
			}
		}
	}

	return true;
}

// +--------------------------------------------------------------------+

int
TerrainApron::CollidesWith(Graphic& o)
{
	return 0;
}

// +--------------------------------------------------------------------+

void
TerrainApron::Update()
{
}

// +--------------------------------------------------------------------+

void
TerrainApron::Illuminate(FColor Ambient, List<SimLight>& Lights)
{
	if (!model || model->GetNumVerts() < 1)
		return;

	Surface* SurfacePtr = model->GetSurfaces().first();
	if (!SurfacePtr)
		return;

	VertexSet* VSet = SurfacePtr->GetVertexSet();
	if (!VSet || VSet->nverts < 1 || !VSet->diffuse || !VSet->nrm)
		return;

	const int32 NumVerts = VSet->nverts;

	// Clear to ambient:
	for (int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex) {
		VSet->diffuse[VertIndex] = Ambient;
	}

	TerrainRegion* Region = terrain ? terrain->GetRegion() : nullptr;
	(void)Region;

	bool bEclipsed = false;

	// For each light:
	ListIter<SimLight> LightIter = Lights;
	while (++LightIter) {
		SimLight* Light = LightIter.value();
		if (!Light)
			continue;

		if (Light->CastsShadow()) {
			bEclipsed = Light->Location().Y < -100.0f;
		}

		if (Light->CastsShadow() && bEclipsed)
			continue;

		FVector LightVec = Light->Location();
		LightVec.Normalize();

		const FColor LightColor = Light->GetColor();

		for (int32 VertIndex = 0; VertIndex < NumVerts; ++VertIndex) {
			const FVector& Normal = VSet->nrm[VertIndex];

			double Value = 0.0;

			if (Light->IsDirectional()) {
				const double Gain = FVector::DotProduct(LightVec, Normal);

				if (Gain > 0.0) {
					Value = Light->Intensity() * (0.85 * Gain);
					Value = FMath::Min(Value, 1.0);
				}
			}

			if (Value <= 0.01)
				continue;

			const float Scale = (float)FMath::Clamp(Value, 0.0, 1.0);

			const uint8 AddR = (uint8)FMath::Clamp(
				(int)FMath::RoundToInt((float)LightColor.R * Scale), 0, 255);

			const uint8 AddG = (uint8)FMath::Clamp(
				(int)FMath::RoundToInt((float)LightColor.G * Scale), 0, 255);

			const uint8 AddB = (uint8)FMath::Clamp(
				(int)FMath::RoundToInt((float)LightColor.B * Scale), 0, 255);

			const FColor Current = VSet->diffuse[VertIndex];

			VSet->diffuse[VertIndex] = FColor(
				(uint8)FMath::Min(255, (int)Current.R + (int)AddR),
				(uint8)FMath::Min(255, (int)Current.G + (int)AddG),
				(uint8)FMath::Min(255, (int)Current.B + (int)AddB),
				Current.A
			);
		}
	}

	InvalidateSurfaceData();
}

// +--------------------------------------------------------------------+

void
TerrainApron::Render(Video* video, DWORD flags)
{
	if (!video || (flags & RENDER_ADDITIVE) || (flags & RENDER_ADD_LIGHT)) return;

	if (!model)
		BuildApron();

	if (scene) {
		Illuminate(scene->Ambient(), scene->Lights());
	}

	double visibility = terrain->GetRegion()->GetWeather().Visibility();
	FLOAT  fog_density = (FLOAT)(terrain->GetRegion()->FogDensity() * 2.5e-5 * 1 / visibility);

	video->SetRenderState(Video::LIGHTING_ENABLE, false);
	video->SetRenderState(Video::SPECULAR_ENABLE, false);
	video->SetRenderState(Video::FOG_ENABLE, true);
	
	const FColor FogColor = terrain->GetRegion()->FogColor();
	video->SetRenderState(Video::FOG_COLOR, FogColor.DWColor());
	video->SetRenderState(Video::FOG_DENSITY, *((DWORD*)&fog_density));

	Solid::Render(video, flags);

	video->SetRenderState(Video::LIGHTING_ENABLE, true);
	video->SetRenderState(Video::SPECULAR_ENABLE, true);
	video->SetRenderState(Video::FOG_ENABLE, false);
}

// +--------------------------------------------------------------------+

int
TerrainApron::CheckRayIntersection(FVector Q, FVector w, double len, FVector& ipt, bool ttpas)
{
	// compute leading edge of ray:
	FVector sun = Q + w * (float)len;

	if (sun.Y < loc.Y)
		return 1;

	return 0;
}
