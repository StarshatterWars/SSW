/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainHaze.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	Atmospheric fog along the horizon
*/

#pragma once

#include "Types.h"
#include "Solid.h"
#include "Geometry.h"

// +--------------------------------------------------------------------+

class TerrainRegion;

// +--------------------------------------------------------------------+

FORCEINLINE uint32 PackColor(const FColor& C)
{
	return (uint32(C.A) << 24) |
		(uint32(C.R) << 16) |
		(uint32(C.G) << 8) |
		(uint32(C.B));
}

class TerrainHaze : public Solid
{
public:
	TerrainHaze();
	virtual ~TerrainHaze();

	virtual void      Render(Video* video, DWORD flags) override;

	virtual int       CheckRayIntersection(FVector pt, FVector vpn, double len, FVector& ipt,
		bool treat_translucent_polys_as_solid = true) override;

	virtual void      UseTerrainRegion(TerrainRegion* tr) { tregion = tr; }

protected:
	TerrainRegion* tregion = nullptr;
};
