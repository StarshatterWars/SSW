/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainClouds.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	Terrain cloud bank graphic (high/low cloud layers rendered above terrain).
*/

#pragma once

#include "Types.h"
#include "Graphic.h"
#include "Geometry.h"
#include "Polygon.h"

// Minimal Unreal include for FVector:
#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class Terrain;
class TerrainRegion;

// Per project mapping:
class SimScene;
class SimLight;

// +--------------------------------------------------------------------+

class TerrainClouds : public Graphic
{
public:
	TerrainClouds(Terrain* terr, int type);
	virtual ~TerrainClouds();

	virtual void      Render(Video* video, DWORD flags) override;
	virtual void      Update() override;

	// accessors:
	virtual int       CollidesWith(Graphic& o) override { return 0; }
	virtual bool      Luminous() const override { return true; }
	virtual bool      Translucent() const override { return true; }

	void              Illuminate(FColor ambient, List<SimLight>& lights);

protected:
	void              BuildClouds();

	Terrain* terrain = nullptr;

	// Converted from Vec3* to FVector*:
	FVector* mverts = nullptr;

	VertexSet* verts = nullptr;
	Poly* polys = nullptr;
	Material          mtl_cloud;
	Material          mtl_shade;

	int               type = 0;
	int               nbanks = 0;
	int               nverts = 0;
	int               npolys = 0;
};
