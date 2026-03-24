/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainHaze.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
*/

#include "TerrainHaze.h"
#include "Terrain.h"
#include "TerrainRegion.h"
#include "SimModel.h"

#include "CoreMinimal.h"

// +====================================================================+
// NOTE:
// The original implementation used a file-static Bitmap. In UE, render
// assets are typically UTexture2D* managed elsewhere. This unit does not
// require a texture reference, so it has been removed.
// +====================================================================+

TerrainHaze::TerrainHaze()
	: tregion(nullptr)
{
}

TerrainHaze::~TerrainHaze()
{
}

void
TerrainHaze::Render(Video* video, DWORD flags)
{
	if (flags & RENDER_ADDITIVE)
		return;

	if (!model)
		return;

	if (!Luminous()) {
		SetLuminous(true);
		model->SetDynamic(true);
	}

	Surface* SurfacePtr = model->GetSurfaces().first();
	if (!SurfacePtr)
		return;

	FColor SkyColor(0, 0, 0, 255);
	FColor FogColor(0, 0, 0, 255);

	if (tregion) {
		SkyColor = tregion->SkyColor().WithAlpha(255);
		FogColor = tregion->FogColor().WithAlpha(255);
	}

	VertexSet* VSet = SurfacePtr->GetVertexSet();
	if (!VSet || !VSet->diffuse || !VSet->loc)
		return;

	for (int32 i = 0; i < VSet->nverts; ++i) {
		// vset->loc is FVector; Y > 0 chooses sky vs fog
		VSet->diffuse[i] = (VSet->loc[i].Y > 0.0f) ? SkyColor : FogColor;
	}

	InvalidateSurfaceData();
	Solid::Render(video, flags);
}

int
TerrainHaze::CheckRayIntersection(FVector Q, FVector w, double len, FVector& ipt, bool ttpas)
{
	return 0;
}
