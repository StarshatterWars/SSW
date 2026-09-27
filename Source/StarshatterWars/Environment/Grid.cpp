/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	Original Author and Studio:
	John DiCamillo / Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Grid.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Tactical Grid class
*/

#include "Grid.h"

#include "Game.h"
#include "Video.h"
#include "View.h"

#include "Math/Vector.h" // FVector

static const FColor DARK_LINE(8, 8, 8);
static const FColor LITE_LINE(16, 16, 16);

// +--------------------------------------------------------------------+

Grid::Grid(int asize, int astep)
	: size(asize), step(astep), drawn(0)
{
	radius = (float)(size * 1.414);
}

Grid::~Grid()
{
}

// +--------------------------------------------------------------------+

int Grid::CollidesWith(Graphic& o)
{
	return 0;
}

// +--------------------------------------------------------------------+

void Grid::Render(Video* video, DWORD flags)
{
	if (!bVisible) {
		return;
	}

	if (!video || hidden) return;

	int   c = 0;
	FColor line;

	for (int i = 0; i <= size; i += step) {
		FVector p1((double)i, 0.0, (double)-size); p1 += Location();
		FVector p2((double)i, 0.0, (double)size); p2 += Location();
		FVector p3((double)-i, 0.0, (double)-size); p3 += Location();
		FVector p4((double)-i, 0.0, (double)size); p4 += Location();

		line = c ? DARK_LINE : LITE_LINE;

		DrawLine(video, p1, p2, line);
		DrawLine(video, p3, p4, line);

		c++;
		if (c > 3) c = 0;
	}

	c = 0;

	for (int i = 0; i <= size; i += step) {
		FVector p1((double)-size, 0.0, (double)i); p1 += Location();
		FVector p2((double)size, 0.0, (double)i); p2 += Location();
		FVector p3((double)-size, 0.0, (double)-i); p3 += Location();
		FVector p4((double)size, 0.0, (double)-i); p4 += Location();

		line = c ? DARK_LINE : LITE_LINE;

		DrawLine(video, p1, p2, line);
		DrawLine(video, p3, p4, line);

		c++;
		if (c > 3) c = 0;
	}
}

void Grid::DrawLine(Video* video, FVector& p1, FVector& p2, FColor grid_color)
{
	// Starshatter render API expects a Vec3 array; FVector is UE-native.
	// If your engine-side Vec3 is now typedef'd/aliased to FVector, this compiles as-is.
	// Otherwise, update Video::DrawLines to accept FVector (recommended during UE port).
	FVector v[2];
	v[0] = p1;
	v[1] = p2;

	video->DrawLines(1, v, grid_color, Video::BLEND_ADDITIVE);
}
