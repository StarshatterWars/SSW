/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         DetailSet.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Level of Detail manager class
*/

#include "DetailSet.h"
#include "Random.h"
#include "Game.h"

// Minimal Unreal support (required: FVector conversions + UE_LOG):
#include "Math/Vector.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterDetailSet, Log, All);

// +--------------------------------------------------------------------+

SimRegion* DetailSet::ref_rgn = 0;
FVector       DetailSet::ref_loc = FVector::ZeroVector;

// +--------------------------------------------------------------------+

DetailSet::DetailSet()
{
	for (int i = 0; i < MAX_DETAIL; i++)
		rad[i] = 0;

	index = -1;
	levels = 0;
	rgn = 0;
	loc = FVector::ZeroVector;
}

DetailSet::~DetailSet()
{
	Destroy();
}

// +--------------------------------------------------------------------+

int
DetailSet::DefineLevel(double r, Graphic* g, FVector* o, FVector* spin_rate)
{
	if (levels < MAX_DETAIL && rep[levels].size() == 0) {
		rad[levels] = r;

		if (g)
			rep[levels].append(g);

		if (o)
			off[levels].append(o);
		else if (g)
			off[levels].append(new FVector(0, 0, 0));

		if (rate.size() == 0) {
			if (spin_rate) {
				rate.append(spin_rate);

				// randomize the initial orientation:
				FVector* initial_spin = new FVector(0, 0, 0);
				if(spin_rate->X != 0.0f)
				{
					initial_spin->X = FMath::FRandRange(-PI, PI);
				}

				if (spin_rate->Y != 0.0f)
				{
					initial_spin->Y = FMath::FRandRange(-PI, PI);
				}

				if (spin_rate->Z != 0.0f)
				{
					initial_spin->Z = FMath::FRandRange(-PI, PI);
				}

				spin.append(initial_spin);
			}
			else {
				rate.append(new FVector(0, 0, 0));
				spin.append(new FVector(0, 0, 0));
			}
		}
		else {
			if (spin_rate)
				rate[rep[levels].size() - 1] = spin_rate;
		}

		levels++;
	}

	return levels - 1;
}

void
DetailSet::AddToLevel(int level, Graphic* g, FVector* Offset, FVector* spin_rate)
{
	if (g && level >= 0 && level < levels) {
		rep[level].append(g);

		if (!Offset)
			Offset = new FVector(0, 0, 0);

		off[level].append(Offset);

		if (spin_rate) {
			int nrep = rep[level].size();
			if (nrep > rate.size())
				rate.append(spin_rate);
			else
				rate[nrep - 1] = spin_rate;
		}

		if (spin.size() < rep[level].size()) {
			FVector* initial_spin = new FVector(0, 0, 0);

			if (spin_rate) {
				// randomize the initial orientation:
				if (spin_rate->X != 0.0f)
					initial_spin->X = FMath::FRandRange(-PI, PI);

				if (spin_rate->Y != 0.0f)
					initial_spin->Y = FMath::FRandRange(-PI, PI);

				if (spin_rate->Z != 0.0f)
					initial_spin->Z = FMath::FRandRange(-PI, PI);
			}

			spin.append(initial_spin);
		}
	}
}

int
DetailSet::NumModels(int level) const
{
	if (level >= 0 && level < levels)
		return rep[level].size();

	return 0;
}

// +--------------------------------------------------------------------+

void
DetailSet::ExecFrame(double seconds)
{
	for (int i = 0; i < spin.size() && i < rate.size(); i++) {
		(*spin[i]) += (*rate[i]) * (float)seconds;
	}
}

// +--------------------------------------------------------------------+

void
DetailSet::SetLocation(SimRegion* r, const FVector& p)
{
	rgn = r;
	loc = p;
}

// +--------------------------------------------------------------------+

void
DetailSet::SetReference(SimRegion* r, const FVector& p)
{
	ref_rgn = r;
	ref_loc = p;
}

// +--------------------------------------------------------------------+

int
DetailSet::GetDetailLevel()
{
	index = 0;

	if (rgn == ref_rgn) {
		const double screen_width = Game::GetScreenWidth();

		const FVector delta = loc - ref_loc;
		const double  distance = (double)delta.Size();

		for (int i = 1; i < levels && rad[i] > 0; i++) {
			const double apparent_feature_size = (rad[i] * screen_width) / distance;

			if (apparent_feature_size > 0.4)
				index = i;
		}
	}

	return index;
}

// +--------------------------------------------------------------------+

Graphic*
DetailSet::GetRep(int level, int n)
{
	if (level >= 0 && level < levels && n >= 0 && n < rep[level].size())
		return rep[level].at(n);

	return 0;
}

FVector
DetailSet::GetOffset(int level, int n)
{
	if (level >= 0 && level < levels && n >= 0 && n < off[level].size())
		return *(off[level].at(n));

	return FVector::ZeroVector;
}

FVector
DetailSet::GetSpin(int level, int n)
{
	if (n >= 0 && n < spin.size())
		return *(spin.at(n));

	return FVector::ZeroVector;
}

// +--------------------------------------------------------------------+

void
DetailSet::Destroy()
{
	for (int i = 0; i < levels; i++) {
		ListIter<Graphic> iter = rep[i];

		while (++iter) {
			Graphic* g = iter.removeItem();
			if (g) {
				g->Destroy();  // this will delete the object (g)
			}
		}

		off[i].destroy();
	}

	rate.destroy();
	spin.destroy();
}

