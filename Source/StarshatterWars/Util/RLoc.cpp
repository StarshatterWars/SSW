/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         RLoc.cpp
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Navigation Point class implementation
*/

#include "RLoc.h"
#include "Random.h"
#include "Math/UnrealMathUtility.h"
// Minimal Unreal include required for FVector math:
#include "Math/Vector.h"

// +----------------------------------------------------------------------+

RLoc::RLoc()
	: RefLoc(nullptr),
	Dex(0.0f),
	DexVar(5.0e3f),
	Az(0.0f),
	AzVar(3.1415f),
	El(0.0f),
	ElVar(0.1f)
{
}

RLoc::RLoc(const FVector& InLoc, double InDistance, double InDistanceVar)
	: Loc(InLoc),
	BaseLoc(InLoc),
	RefLoc(nullptr),
	Dex((float)InDistance),
	DexVar((float)InDistanceVar),
	Az(0.0f),
	AzVar(3.1415f),
	El(0.0f),
	ElVar(0.1f)
{
}

RLoc::RLoc(RLoc* InRefLoc, double InDistance, double InDistanceVar)
	: RefLoc(InRefLoc),
	Dex((float)InDistance),
	DexVar((float)InDistanceVar),
	Az(0.0f),
	AzVar(3.1415f),
	El(0.0f),
	ElVar(0.1f)
{
}

RLoc::RLoc(const RLoc& R)
	: Loc(R.Loc),
	BaseLoc(R.BaseLoc),
	RefLoc(R.RefLoc),
	Dex(R.Dex),
	DexVar(R.DexVar),
	Az(R.Az),
	AzVar(R.AzVar),
	El(R.El),
	ElVar(R.ElVar)
{
}

RLoc::~RLoc()
{
}

// +----------------------------------------------------------------------+

const FVector&
RLoc::Location()
{
	if (RefLoc || Dex > 0.0f)
		Resolve();

	return Loc;
}

// +----------------------------------------------------------------------+

void
RLoc::Resolve()
{
	if (RefLoc) {
		BaseLoc = RefLoc->Location();
		RefLoc = nullptr;
	}

	if (Dex > 0.0f) {
		const double D = (double)Dex + (double)FMath::FRandRange(-(float)DexVar, (float)DexVar);
		const double A = (double)Az + (double)FMath::FRandRange(-(float)AzVar, (float)AzVar);
		const double E = (double)El + (double)FMath::FRandRange(-(float)ElVar, (float)ElVar);

		// Original Starshatter coordinate intent preserved:
		// x = d*sin(a), y = -d*cos(a), z = d*sin(e)
		const FVector Offset(
			(float)(D * sin(A)),
			(float)(D * -cos(A)),
			(float)(D * sin(E))
		);

		Loc = BaseLoc + Offset;
		Dex = 0.0f;
	}
	else {
		Loc = BaseLoc;
	}
}

// +----------------------------------------------------------------------+

void
RLoc::SetBaseLocation(const FVector& InBaseLoc)
{
	BaseLoc = InBaseLoc;
	Loc = InBaseLoc;
}
