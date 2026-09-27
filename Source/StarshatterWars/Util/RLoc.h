/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGen.lib
	FILE:         RLoc.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Relative Location (RLoc) class declaration
*/

#pragma once

#include "Types.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class RLoc
{
public:
	RLoc();
	RLoc(const FVector& InLoc, double InDistance, double InDistanceVar = 5e3);
	RLoc(RLoc* InRefLoc, double InDistance, double InDistanceVar = 5e3);
	RLoc(const RLoc& R);
	~RLoc();

	// accessors:
	const FVector& Location();
	const FVector& BaseLocation() const { return BaseLoc; }
	RLoc* ReferenceLoc() const { return RefLoc; }
	double             Distance()     const { return Dex; }
	double             DistanceVar()  const { return DexVar; }
	double             Azimuth()      const { return Az; }
	double             AzimuthVar()   const { return AzVar; }
	double             Elevation()    const { return El; }
	double             ElevationVar() const { return ElVar; }

	void               Resolve();

	// mutators:
	void               SetBaseLocation(const FVector& InBaseLoc);
	void               SetReferenceLoc(RLoc* InRef) { RefLoc = InRef; }
	void               SetDistance(double InDistance) { Dex = (float)InDistance; }
	void               SetDistanceVar(double InVar) { DexVar = (float)InVar; }
	void               SetAzimuth(double InAzimuth) { Az = (float)InAzimuth; }
	void               SetAzimuthVar(double InVar) { AzVar = (float)InVar; }
	void               SetElevation(double InElevation) { El = (float)InElevation; }
	void               SetElevationVar(double InVar) { ElVar = (float)InVar; }

private:
	FVector            Loc = FVector::ZeroVector;
	FVector            BaseLoc = FVector::ZeroVector;
	RLoc* RefLoc = nullptr;

	float              Dex = 0.0f;
	float              DexVar = 0.0f;
	float              Az = 0.0f;
	float              AzVar = 0.0f;
	float              El = 0.0f;
	float              ElVar = 0.0f;
};

// +--------------------------------------------------------------------+
