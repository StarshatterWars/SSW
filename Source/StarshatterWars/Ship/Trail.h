/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Trail.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Missile Trail (Graphic) class
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Polygon.h"
#include "SimObject.h"
#include "Graphic.h"

// Unreal (minimal):
#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

class Bitmap;
class Video;
struct Poly;
struct VertexSet;
struct Material;

// +--------------------------------------------------------------------+

class Trail : public Graphic
{
public:
	Trail(Bitmap* tex, int n = 512);
	virtual ~Trail();

	virtual void      UpdateVerts(const FVector& CamPos);
	virtual void      Render(Video* video, DWORD flags);
	virtual void      AddPoint(const FVector& V);
	virtual double    AverageLength();

	virtual void      SetWidth(double w) { width = w; }
	virtual void      SetDim(int d) { dim = d; }

protected:
	int            ntrail;
	int            maxtrail;
	FVector* trail;

	double         length;
	double         width;
	int            dim;

	int            npolys, nverts;
	Poly* polys;
	VertexSet* verts;
	Bitmap* texture;
	Material       mtl;

	double         length0, length1;
	double         last_point_time;
};
