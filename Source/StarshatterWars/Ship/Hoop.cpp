/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Hoop.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	ILS Hoop (HUD display) class
*/

#include "Hoop.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include <cstring>

#include "Game.h"
#include "DataLoader.h"
#include "View.h"
#include "SimModel.h"
#include "Surface.h"
#include "Segment.h"

static FColor ils_color;


// +--------------------------------------------------------------------+

Hoop::Hoop()
	: hoop_texture(nullptr)
	, mtl(nullptr)
	, width(360)
	, height(180)
{
	foreground = 1;
	radius = (float)width;

	DataLoader* loader = DataLoader::GetLoader();

	loader->SetDataPath("HUD/");
	loader->LoadTexture("ILS.pcx", hoop_texture, /*Bitmap::BMP_TRANSLUCENT*/ 0);
	loader->SetDataPath(0);

	CreatePolys();
}

Hoop::~Hoop()
{
}

// +--------------------------------------------------------------------+

void
Hoop::SetColor(FColor c)
{
	ils_color = c;
}

// +--------------------------------------------------------------------+

void
Hoop::CreatePolys()
{
	Material* LocalMtl = new Material;

	LocalMtl->tex_diffuse = hoop_texture;
	LocalMtl->blend = Material::MTL_ADDITIVE;

	int w = width / 2;
	int h = height / 2;

	model = new SimModel;
	own_model = 1;

	Surface* surface = new Surface;

	surface->SetName("hoop");
	surface->CreateVerts(4);
	surface->CreatePolys(2);

	VertexSet* vset = surface->GetVertexSet();
	Poly* polys = surface->GetPolys();

	std::memset(polys, 0, sizeof(Poly) * 2);

	for (int i = 0; i < 4; i++) {
		int   x = w;
		int   y = h;
		float u = 0;
		float v = 0;

		if (i == 0 || i == 3)
			x = -x;
		else
			u = 1;

		if (i < 2)
			y = -y;
		else
			v = 1;

		vset->loc[i] = FVector((float)x, (float)y, 0.0f);
		vset->nrm[i] = FVector::ZeroVector;

		vset->tu[i] = u;
		vset->tv[i] = v;
	}

	for (int i = 0; i < 2; i++) {
		Poly& poly = polys[i];

		poly.nverts = 4;
		poly.vertex_set = vset;
		poly.material = LocalMtl;

		poly.verts[0] = i ? 3 : 0;
		poly.verts[1] = i ? 2 : 1;
		poly.verts[2] = i ? 1 : 2;
		poly.verts[3] = i ? 0 : 3;

		poly.plane = Plane(vset->loc[poly.verts[0]],
			vset->loc[poly.verts[2]],
			vset->loc[poly.verts[1]]);

		surface->AddIndices(6);
	}

	// then assign them to cohesive segments:
	Segment* segment = new Segment;
	segment->npolys = 2;
	segment->polys = &polys[0];
	segment->material = segment->polys->material;

	surface->GetSegments().append(segment);

	model->AddSurface(surface);

	mtl = LocalMtl;

	SetLuminous(true);
}

// +--------------------------------------------------------------------+

void
Hoop::Update()
{
	if (mtl)
		mtl->Ke = ils_color;

	if (model && luminous) {

		ListIter<Surface> SurfaceIter = model->GetSurfaces();
		while (++SurfaceIter) {
			Surface* SurfacePtr = SurfaceIter.value();
			if (!SurfacePtr)
				continue;

			VertexSet* VSet = SurfacePtr->GetVertexSet();
			if (!VSet || !VSet->diffuse || VSet->nverts < 1)
				continue;

			for (int32 VertexIndex = 0; VertexIndex < VSet->nverts; ++VertexIndex) {
				VSet->diffuse[VertexIndex] = ils_color; 
			}
		}

		InvalidateSurfaceData();
	}
}
