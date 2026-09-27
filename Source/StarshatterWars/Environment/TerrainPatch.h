/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainPatch.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

	OVERVIEW
	========
	A Single Multi-LOD Section of a Terrain Object
*/

#pragma once

#include "Types.h"
#include "Solid.h"
#include "Geometry.h"
#include "Polygon.h"

// Minimal UE include needed for FVector:
#include "Math/Vector.h"
#include "Math/Color.h"

// +--------------------------------------------------------------------+

class UTexture2D;

class SimProjector;
class Terrain;
class Water;
class SimLight;

// +--------------------------------------------------------------------+

class TerrainPatch : public Solid
{
public:
	TerrainPatch(Terrain* terrain,
		const Bitmap* patch, const Rect& rect,
		const FVector& p1, const FVector& p2);

	TerrainPatch(Terrain* terrain,
		const Rect& rect,
		const FVector& p1, const FVector& p2,
		double sea_level);

	virtual ~TerrainPatch();

	virtual void      SelectDetail(SimProjector* projector);
	virtual void      Render(Video* video, DWORD flags);

	virtual int       CollidesWith(Graphic& o);

	// accessors:
	double            Scale()         const { return scale; }
	double            MountainScale() const { return mtnscale; }
	double            SeaLevel()      const { return base; }
	double            MinHeight()     const { return min_height; }
	double            MaxHeight()     const { return max_height; }
	bool              IsWater()       const { return water != nullptr; }

	void              UpdateSurfaceWaves(FVector& eyePos);

	void              SetScales(double scale, double mtnscale, double base);
	void              SetDetailLevel(int nd);

	virtual int       CheckRayIntersection(FVector pt, FVector vpn, double len, FVector& ipt,
		bool treat_translucent_polys_as_solid = true);

	double            Height(double x, double y) const;
	DWORD             BlendValue(double y);
	int               CalcLayer(Poly* p);
	void              Illuminate(FColor ambient, List<SimLight>& lights);

protected:
	virtual bool      BuildDetailLevel(int level);

	enum { MAX_LOD = 8 };

	Terrain* terrain = nullptr;
	int               patch_size = 0;
	int               ndetail = 0;
	int               max_detail = 0;
	int               terrain_width = 0;

	Rect              rect;
	float* heights = nullptr;

	SimModel* detail_levels[MAX_LOD] = { nullptr };
	List<Material>    materials;
	Water* water = nullptr;

	double            scale = 1.0;
	double            mtnscale = 1.0;
	double            base = 0.0;
	double            size = 0.0;
	float             min_height = 0.0f;
	float             max_height = 0.0f;
};
