/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         DetailSet.h
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Level of Detail Manager class
*/
#pragma once
#include "Types.h"
#include "Geometry.h"
#include "Graphic.h"
#include "List.h"

// Minimal Unreal include (required by request: convert Point/Vec3 to FVector):
#include "Math/Vector.h"

// +--------------------------------------------------------------------+
// Forward declarations

class UTexture2D;

class Sim;
class SimRegion;
class Graphic;

// +--------------------------------------------------------------------+

class DetailSet
{
public:
	enum { MAX_DETAIL = 4 };

	DetailSet();
	virtual ~DetailSet();

	int            DefineLevel(double r, Graphic* g = 0, FVector* offset = 0, FVector* spin = 0);
	void           AddToLevel(int level, Graphic* g, FVector* offset = 0, FVector* spin = 0);
	int            NumLevels() const { return levels; }
	int            NumModels(int level) const;

	void           ExecFrame(double seconds);
	void           SetLocation(SimRegion* rgn, const FVector& loc);
	static void    SetReference(SimRegion* rgn, const FVector& loc);

	int            GetDetailLevel();
	Graphic* GetRep(int level, int n = 0);
	FVector        GetOffset(int level, int n = 0);
	FVector        GetSpin(int level, int n = 0);
	void           Destroy();

protected:
	List<Graphic>     rep[MAX_DETAIL];
	List<FVector>     off[MAX_DETAIL];
	double            rad[MAX_DETAIL];

	List<FVector>     spin;
	List<FVector>     rate;

	int               index;
	int               levels;
	SimRegion* rgn;
	FVector           loc;

	static SimRegion* ref_rgn;
	static FVector    ref_loc;
};

