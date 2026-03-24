/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Trail.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Missile Trail representation class
*/

#include "Trail.h"

#include "Weapon.h"
#include "Sim.h"

#include "Game.h"
#include "SimLight.h"
#include "Bitmap.h"
#include "DataLoader.h"
#include "Sound.h"

// Unreal:
#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"   // FMath

// +--------------------------------------------------------------------+

Trail::Trail(Bitmap* tex, int n)
	: ntrail(0)
	, maxtrail(n)
	, trail(0)
	, length(0.0)
	, width(6.0)
	, dim(1)
	, npolys(0)
	, nverts(0)
	, polys(0)
	, verts(0)
	, texture(tex)
	, mtl()
	, length0(0.0)
	, length1(0.0)
	, last_point_time(0.0)
{
	trans = true;
	luminous = true;

	// Material setup (FColor in UE port):
	mtl.Kd = FColor::White;
	mtl.tex_diffuse = texture;
	mtl.tex_emissive = texture;
	mtl.blend = Video::BLEND_ADDITIVE;
	mtl.luminous = true;

	if (maxtrail < 4)
		maxtrail = 512;

	// Unreal does not support MemDebug (__FILE__, __LINE__) overloads:
	trail = new FVector[maxtrail];

	verts = new VertexSet(maxtrail * 2);
	verts->Clear();
	verts->nverts = 0;

	for (int i = 0; i < maxtrail * 2; i++) {

		// Diffuse: full white, opaque (matches original D3D packed color)
		verts->diffuse[i] = FColor(255, 255, 255, 255);

		// Specular: black, opaque (matches D3DCOLOR_RGBA(0,0,0,255))
		verts->specular[i] = FColor(0, 0, 0, 255);

		// Texture coordinates
		verts->tu[i] = 0.0f;
		verts->tv[i] = (i & 1) ? 1.0f : 0.0f;
	}

	polys = new Poly[maxtrail];

	for (int i = 0; i < maxtrail; i++) {
		polys[i].vertex_set = verts;
		polys[i].nverts = 4;
		polys[i].material = &mtl;
	}

	npolys = 0;
	nverts = 0;
	width = 6.0;
	length = 0.0;
	dim = 3;

	last_point_time = 0.0;
}

Trail::~Trail()
{
	delete[] trail;
	delete[] polys;
	delete   verts;
}

void Trail::UpdateVerts(const FVector& CamPos)
{
	if (ntrail < 2 || !verts || !trail)
		return;

	int32 Bright = 255 - dim * ntrail;

	const FVector Head = trail[1] + loc;
	const FVector Tail = trail[0] + loc;

	const FVector VCam = CamPos - Head;

	// vtmp = vcam cross (head - tail)
	FVector VTmp = FVector::CrossProduct(VCam, (Head - Tail));
	VTmp.Normalize();

	const FVector VLat = VTmp * (float)(width + (0.1 * width * ntrail));

	verts->loc[0] = Tail - VLat;
	verts->loc[1] = Tail + VLat;

	verts->diffuse[0] = FColor(0, 0, 0, 0);
	verts->diffuse[1] = FColor(0, 0, 0, 0);

	for (int32 i = 0; i < ntrail - 1; ++i) {
		Bright += dim;

		const FVector SegHead = trail[i + 1] + loc;
		const FVector SegTail = trail[i] + loc;

		const FVector SegVCam = CamPos - SegHead;

		FVector SegVTmp = FVector::CrossProduct(SegVCam, (SegHead - SegTail));
		SegVTmp.Normalize();

		float TrailWidth = (float)(width + (ntrail - i) * width * 0.1);
		if (i == ntrail - 2)
			TrailWidth = (float)(width * 0.7);

		const FVector SegVLat = SegVTmp * TrailWidth;

		const int32 V0 = 2 * i + 2;
		const int32 V1 = 2 * i + 3;

		verts->loc[V0] = SegHead - SegVLat;
		verts->loc[V1] = SegHead + SegVLat;

		if (Bright <= 0) {
			verts->diffuse[V0] = FColor(0, 0, 0, 0);
			verts->diffuse[V1] = FColor(0, 0, 0, 0);
		}
		else {
			const uint8 Clamped = (uint8)FMath::Clamp(Bright, 0, 255);
			verts->diffuse[V0] = FColor(Clamped, Clamped, Clamped, Clamped);
			verts->diffuse[V1] = FColor(Clamped, Clamped, Clamped, Clamped);

			//const uint8 A = (uint8)FMath::Clamp(Bright, 0, 255);
			//verts->diffuse[V0] = FColor(255, 255, 255, A);
			//verts->diffuse[V1] = FColor(255, 255, 255, A);
		}
	}
}


void
Trail::Render(Video* video, DWORD flags)
{
	if (!npolys)
		return;

	if ((flags & RENDER_ADDITIVE) == 0)
		return;

	if (video && life) {
		const Camera* Cam = video->GetCamera();

		if (Cam)
			UpdateVerts(Cam->Pos());

		video->DrawPolys(npolys, polys);
	}
}

void
Trail::AddPoint(const FVector& V)
{
	if (ntrail >= maxtrail - 1)
		return;

	const double RealTime = Game::RealTime() / 1000.0;

	if (ntrail == 0) {
		radius = 1000.0f;
	}
	else {
		radius = (float)(V - loc).Length();
	}

	// Just adjust the last point:
	if (ntrail > 1 && (RealTime - last_point_time) < 0.05) {
		trail[ntrail - 1] = V;
	}

	// Add a new point:
	else {
		last_point_time = RealTime;
		trail[ntrail++] = V;

		nverts += 2;
		verts->nverts = nverts;

		if (ntrail > 1) {
			length0 = length1;
			length1 = length0 + (trail[ntrail - 1] - trail[ntrail - 2]).Length();

			polys[npolys].vertex_set = verts;
			polys[npolys].nverts = 4;

			polys[npolys].verts[0] = nverts - 4;
			polys[npolys].verts[1] = nverts - 2;
			polys[npolys].verts[2] = nverts - 1;
			polys[npolys].verts[3] = nverts - 3;

			const float tu1 = (float)(length1 / 250.0);

			verts->tu[2 * ntrail - 1] = tu1;
			verts->tu[2 * ntrail - 2] = tu1;

			npolys++;
		}
	}
}

double
Trail::AverageLength()
{
	if (ntrail < 2)
		return 0.0;

	double Avg = 0.0;

	for (int i = 0; i < ntrail - 1; i++) {
		Avg += (trail[i + 1] - trail[i]).Length();
	}

	Avg /= ntrail;

	return Avg;
}
