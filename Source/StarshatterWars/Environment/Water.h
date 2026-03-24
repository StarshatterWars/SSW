/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo / Destroyer Studios LLC
	Copyright © 1997-2006. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         Water.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Water surface effect w/ reflection and refraction
*/

#pragma once

#include "Geometry.h"
#include "Polygon.h"
#include "Color.h"

// Minimal Unreal include for FVector (Point/Vec3 -> FVector):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

struct WATER_SURFACE;

// +--------------------------------------------------------------------+

class Water
{
public:
	Water();
	virtual ~Water();

	virtual void   Init(int nVerts, float size, float depth);
	virtual void   CalcWaves(double seconds);
	virtual void   UpdateSurface(FVector& eyePos, VertexSet* vset);

protected:
	float          size;
	float          depth;
	float          scaleTex;
	float          avgHeight;

	DWORD          nVertices;

	WATER_SURFACE* surface;
	float* waves;
	float          offsets[16];
};

