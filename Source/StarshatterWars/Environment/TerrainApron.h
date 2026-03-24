/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         TerrainApron.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	A Single Edge Section of a Terrain Object
*/

#pragma once

#include "Types.h"
#include "Solid.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class Terrain;
class Bitmap;
struct Rect;
class Video;
class Graphic;
class SimLight;

// +--------------------------------------------------------------------+

class TerrainApron : public Solid
{
public:
	TerrainApron(
		Terrain* terrain,
		const Bitmap* patch,
		const Rect& rect,
		const FVector& p1,
		const FVector& p2
	);

	virtual ~TerrainApron();

	virtual void   Render(Video* video, DWORD flags);
	virtual void   Update();

	virtual int    CollidesWith(Graphic& o);
	virtual bool   Luminous()    const { return false; }
	virtual bool   Translucent() const { return false; }

	// accessors:
	double         Scale()         const { return scale; }
	double         MountainScale() const { return mtnscale; }
	double         SeaLevel()      const { return base; }
	void           SetScales(double scale, double mtnscale, double base);

	void           Illuminate(FColor ambient, List<SimLight>& lights);

	virtual int    CheckRayIntersection(
		FVector pt,
		FVector vpn,
		double len,
		FVector& ipt,
		bool treat_translucent_polys_as_solid = true
	);

protected:
	virtual bool   BuildApron();

	Terrain* terrain;
	int            nverts;
	int            npolys;
	int            terrain_width;

	Rect           rect;
	float* heights;

	double         scale;
	double         mtnscale;
	double         base;
	double         size;
};
