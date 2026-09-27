/*  Project Starshatter Wars
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    Foundation
	FILE:         RLoc.h
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Utility functions for generating random numbers and locations.
*/


#include "Random.h"

// Minimal Unreal support (FVector conversions + UE_LOG):
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// +----------------------------------------------------------------------+

void RandomInit()
{
	FMath::RandInit(static_cast<int32>(FDateTime::Now().GetTicks() % MAX_int32));
}

// +----------------------------------------------------------------------+

Point RandomDirection()
{
	Point p = Point(rand() - 16384, rand() - 16384, 0);
	p.Normalize();
	return p;
}

// +----------------------------------------------------------------------+

Point RandomPoint()
{
	Point p = Point(rand() - 16384, rand() - 16384, 0);
	p.Normalize();
	p *= 15e3 + rand() / 3;
	return p;
}

// +----------------------------------------------------------------------+

Vec3 RandomVector(double radius)
{
	Vec3 v = Vec3(rand() - 16384, rand() - 16384, rand() - 16384);
	v.Normalize();

	if (radius > 0)
		v *= (float)radius;
	else
		v *= (float)RandomDouble(radius / 3, radius);

	return v;
}

// +----------------------------------------------------------------------+

double RandomDouble(double min, double max)
{
	double delta = max - min;
	double r = delta * rand() / 32768.0;

	return min + r;
}

// +----------------------------------------------------------------------+

int RandomIndex()
{
	static int index = 0;
	static int table[16] = { 0, 9, 4, 7, 14, 11, 2, 12, 1, 5, 13, 8, 6, 10, 3, 15 };

	int r = 1 + ((rand() & 0x0700) >> 8);
	index += r;
	if (index > 1e7) index = 0;
	return table[index % 16];
}

// +----------------------------------------------------------------------+

bool RandomChance(int wins, int tries)
{
	double fraction = 256.0 * wins / tries;
	double r = (rand() >> 4) & 0xFF;

	return r < fraction;
}

// +----------------------------------------------------------------------+

int RandomSequence(int current, int range)
{
	if (range > 1) {
		int step = (int)RandomDouble(1, range - 1);
		return (current + step) % range;
	}

	return current;
}

// +----------------------------------------------------------------------+

int RandomShuffle(int count)
{
	static int  set_size = -1;
	static BYTE set[256];
	static int  index = -1;

	if (count < 0 || count > 250)
		return 0;

	if (set_size != count) {
		set_size = count;
		index = -1;
	}

	// need to reshuffle
	if (index < 0 || index > set_size - 1) {
		// set up the deck
		int tmp[256];
		for (int i = 0; i < 256; i++)
			tmp[i] = i;

		// shuffle the cards
		for (int i = 0; i < set_size; i++) {
			int n = (int)RandomDouble(0, set_size);
			int tries = set_size;
			while (tmp[n] < 0 && tries--) {
				n = (n + 1) % set_size;
			}

			if (tmp[n] >= 0) {
				set[i] = tmp[n];
				tmp[n] = -1;
			}
			else {
				set[i] = 0;
			}
		}

		index = 0;
	}

	return set[index++];
}

FVector GetRandomPoint()
{
	FVector P(
		FMath::FRandRange(-16384.0f, 16384.0f),
		FMath::FRandRange(-16384.0f, 16384.0f),
		0.0f
	);

	P.Normalize();

	const float Distance = 15000.0f + FMath::FRandRange(0.0f, 32767.0f / 3.0f);
	return P * Distance;
}

FVector GetRandomDirection()
{
	FVector P(
		FMath::FRandRange(-16384.0f, 16384.0f),
		FMath::FRandRange(-16384.0f, 16384.0f),
		0.0f
	);

	return P.GetSafeNormal();
}

bool GetRandomChance(int32 Wins, int32 Tries)
{
	const float Fraction = 256.0f * Wins / Tries;
	const int32 R = (FMath::Rand() >> 4) & 0xFF;

	return R < Fraction;
}

int32 GetRandomIndex()
{
	static int32 Index = 0;
	static int32 Table[16] = { 0, 9, 4, 7, 14, 11, 2, 12, 1, 5, 13, 8, 6, 10, 3, 15 };

	const int32 R = 1 + ((FMath::Rand() & 0x0700) >> 8);
	Index += R;

	if (Index > 10000000)
	{
		Index = 0;
	}

	return Table[Index % 16];
}





