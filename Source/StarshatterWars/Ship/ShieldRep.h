/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShieldRep.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	ShieldRep Solid class
*/

#pragma once

#include "Types.h"
#include "Solid.h"

// Minimal Unreal include required by conversion of Vec3/Point -> FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

struct ShieldHit;
class SimShot;

// +--------------------------------------------------------------------+

class ShieldRep : public Solid
{
public:
	ShieldRep();
	virtual ~ShieldRep();

	// operations
	virtual void   Render(Video* video, DWORD flags);
	virtual void   Energize(double seconds, bool bubble = false);
	int            ActiveHits() const { return nhits; }
	virtual void   Hit(FVector impact, SimShot* shot, double damage = 0);
	virtual void   TranslateBy(const FVector& ref);
	virtual void   Illuminate();

protected:
	int        nhits = 0;
	ShieldHit* hits = nullptr;
	FVector    true_eye_point = FVector::ZeroVector;
	bool       bubble = false;
};
