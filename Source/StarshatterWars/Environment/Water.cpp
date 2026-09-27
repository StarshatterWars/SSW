/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo / Destroyer Studios LLC
	Copyright © 1997-2006. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         Water.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Water surface effect w/ reflection and caustics
*/

#include "Water.h"

// Starshatter core:
#include "Random.h"

// Unreal (minimal):
#include "Math/UnrealMathUtility.h"   // FMath::Sin, FMath::Cos, FMath::Acos, etc.
#include "Logging/LogMacros.h"
#include <cstring>                   // std::strncpy
#include <algorithm>                 // std::max

// +--------------------------------------------------------------------+

struct WATER_REFRACT
{
	// Vrefract = (V + refract * N) * norm
	float refract;
	float refractNorm;
	DWORD diffuse;
};

struct WATER_SURFACE
{
	float   height;
	FVector normal;
};

// +--------------------------------------------------------------------+
//
// Unreal does not support MSVC inline asm in this context. Use a safe cast.
//
static FORCEINLINE int f2i(float flt)
{
	return (int)flt;
}

// +--------------------------------------------------------------------+

static WATER_REFRACT RefractionTable[512];
static bool refractInit = false;

static const int   WAVE_SIZE = 256;
static const DWORD WAVE_MASK = 0xff;

// +--------------------------------------------------------------------+

Water::Water()
	: size(0)
	, depth(0)
	, scaleTex(1)
	, avgHeight(0)
	, nVertices(0)
	, surface(nullptr)
	, waves(nullptr)
{
	FMemory::Memzero(offsets, sizeof(offsets));
}

Water::~Water()
{
	delete[] surface;
	delete[] waves;
}

// +--------------------------------------------------------------------+

void
Water::Init(int n, float s, float d)
{
	size = s;
	depth = d;
	scaleTex = (size != 0.0f) ? (1.0f / size) : 0.0f;

	// Calculate number of vertices
	nVertices = (DWORD)std::max(n, 0);

	// Create refraction table
	if (!refractInit) {
		WATER_REFRACT* refract = &RefractionTable[256];

		for (UINT u = 0; u < 256; u++) {
			const float fCos0 = (float)u / 256.0f;
			const float f0 = FMath::Acos(fCos0);
			const float fSin0 = FMath::Sin(f0);

			const float fSin1 = fSin0 / 1.333f; // water
			const float f1 = FMath::Asin(fSin1);
			const float fCos1 = FMath::Cos(f1);

			refract[u].refract = (fSin1 != 0.0f) ? (fSin0 / fSin1 * fCos1 - fCos0) : 0.0f;
			refract[u].refractNorm = (fSin0 != 0.0f) ? (-fSin1 / fSin0) : 0.0f;
			refract[u].diffuse = ((((0xff - u) * (0xff - u) * (0xff - u)) << 8) & 0xff000000);

			RefractionTable[u] = RefractionTable[256];
		}

		refractInit = true;
	}

	// Create maps
	if (surface) {
		delete[] surface;
		surface = nullptr;
	}

	if (nVertices > 0) {
		surface = new WATER_SURFACE[nVertices * nVertices];
		FMemory::Memzero(surface, nVertices * nVertices * sizeof(WATER_SURFACE));
	}

	if (waves) {
		delete[] waves;
		waves = nullptr;
	}

	waves = new float[WAVE_SIZE * 4];

	const double f = 1.0 / (double)WAVE_SIZE;
	for (int i = 0; i < WAVE_SIZE; i++) {
		const double s0 = FMath::Sin((float)(2.0 * PI * i * f));
		const double s1 = FMath::Sin((float)(4.0 * PI * i * f));
		const double s2 = FMath::Sin((float)(6.0 * PI * i * f));
		const double s3 = FMath::Sin((float)(8.0 * PI * i * f));

		waves[0 * WAVE_SIZE + i] = (float)(1.8 * s0 * s0 - 0.9);
		waves[1 * WAVE_SIZE + i] = (float)(1.6 * s1 * s1 - 0.8);
		waves[2 * WAVE_SIZE + i] = (float)(0.4 * s2);
		waves[3 * WAVE_SIZE + i] = (float)(0.8 * s3 * s3 - 0.4);
	}

	for (int i = 0; i < 4; i++) {
		offsets[i] = FMath::FRandRange(0.0f, (float)WAVE_SIZE);
	}

	offsets[4] = 12.45f;
	offsets[5] = 14.23f;
	offsets[6] = 16.72f;
	offsets[7] = 20.31f;
}

// +--------------------------------------------------------------------+

void
Water::CalcWaves(double seconds)
{
	if (!surface || !waves || nVertices < 2)
		return;

	int  i, n[4];
	const UINT SIZE = (UINT)nVertices;
	const UINT STEP = WAVE_SIZE / (SIZE - 1);
	const UINT STEP2 = STEP / 2;
	const UINT AREA = SIZE * SIZE;

	UINT x, y;

	for (i = 0; i < 4; i++) {
		n[i] = (int)offsets[i];
	}

	WATER_SURFACE* pSurf = surface;

	// compute heights
	for (y = 0; y < SIZE; y++) {
		for (x = 0; x < SIZE; x++) {
			float h = 0;
			h += waves[(((n[0] + x * STEP - y * STEP2) & WAVE_MASK) + 0 * WAVE_SIZE)];
			h += waves[(((n[1] + x * STEP2 + y * STEP) & WAVE_MASK) + 1 * WAVE_SIZE)];
			h += waves[(((n[2] + x * STEP) & WAVE_MASK) + 2 * WAVE_SIZE)];
			h += waves[(((n[3] + y * STEP) & WAVE_MASK) + 3 * WAVE_SIZE)];

			pSurf->height = h * depth;
			++pSurf;
		}
	}

	// compute normals
	UINT uXN, uX0, uXP;
	UINT uYN, uY0, uYP;

	uYP = AREA - SIZE;
	uY0 = 0;
	uYN = SIZE;

	for (y = 0; y < SIZE; y++) {
		uXP = SIZE - 1;
		uX0 = 0;
		uXN = 1;

		for (x = 0; x < SIZE; x++) {
			FVector vecN(0, 0, 0);
			float f;

			f = surface[uXN + uYN].height - surface[uXP + uYP].height; vecN.X = vecN.Z = f;
			f = surface[uX0 + uYN].height - surface[uX0 + uYP].height; vecN.Z += f;
			f = surface[uXP + uYN].height - surface[uXN + uYP].height; vecN.X -= f; vecN.Z += f;
			f = surface[uXN + uY0].height - surface[uXP + uY0].height; vecN.X += f;

			vecN.Y = -15.0f * depth;

			if (!vecN.Normalize()) {
				vecN = FVector(0, 1, 0);
			}

			surface[uX0 + uY0].normal = vecN * -1.0f;

			uXP = uX0;
			uX0 = uXN;
			uXN = (uXN + 1) % SIZE;
		}

		uYP = uY0;
		uY0 = uYN;
		uYN = (uYN + SIZE) % AREA;
	}

	// update offsets
	for (i = 0; i < 4; i++) {
		offsets[i] += (float)(offsets[i + 4] * seconds);

		if (offsets[i] > WAVE_SIZE)
			offsets[i] -= WAVE_SIZE;
	}
}

// +--------------------------------------------------------------------+

void
Water::UpdateSurface(FVector& EyePos, VertexSet* VSet)
{
	(void)EyePos;

	if (!surface || !VSet || nVertices < 2 || (UINT)VSet->nverts < (UINT)(nVertices * nVertices))
		return;

	const UINT Size = (UINT)nVertices;

	WATER_SURFACE* SurfPtr = surface;

	FVector* LocPtr = VSet->loc;
	FVector* NormPtr = VSet->nrm;

	// VertexSet::diffuse is now FColor* in your UE port:
	FColor* DiffPtr = VSet->diffuse;

	float* TuPtr = VSet->tu;
	float* TvPtr = VSet->tv;

	const float Inc = 1.0f / (float)(Size - 1);

	float Fx = 0.0f;
	float Fz = 0.0f;

	for (UINT y = 0; y < Size; y++) {
		for (UINT x = 0; x < Size; x++) {

			// update vertex height and normal
			LocPtr->Y += SurfPtr->height;
			*NormPtr = SurfPtr->normal;

			// (Refraction/caustics block intentionally retained as comment from original)

			Fx += Inc;

			++SurfPtr;
			++LocPtr;
			++NormPtr;

			// Maintain original pointer-walk behavior:
			if (DiffPtr) ++DiffPtr;
			if (TuPtr)   ++TuPtr;
			if (TvPtr)   ++TvPtr;
		}

		Fx = 0.0f;
		Fz += Inc;
	}
}
