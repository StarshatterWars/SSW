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

#pragma once

#include "CoreMinimal.h"
#include "Types.h"
#include "Geometry.h"

/**
 * 
 */
// +----------------------------------------------------------------------+

	void     RandomInit();
	Point    RandomDirection();
	Point    RandomPoint();
	Vec3     RandomVector(double radius);
	double   RandomDouble(double min = 0, double max = 1);
	int      RandomIndex();
	bool     RandomChance(int wins = 1, int tries = 2);
	int      RandomSequence(int current, int range);
	int      RandomShuffle(int count);

	// +----------------------------------------------------------------------+

