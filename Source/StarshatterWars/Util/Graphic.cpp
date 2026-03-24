/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         Graphic.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	Abstract 3D Graphic Object
*/

#include "Graphic.h"
#include "SimScene.h"
#include "SimProjector.h"

// Minimal Unreal support (FVector conversions + UE_LOG):
#include "Math/Vector.h"
#include "Logging/LogMacros.h"

// C runtime:
#include <cstring>
#include <cmath>

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterGraphic, Log, All);

// +--------------------------------------------------------------------+

int Graphic::id_key = 1;

// +--------------------------------------------------------------------+

Graphic::Graphic()
	: id(id_key++),
	loc(0.0f, 0.0f, 0.0f),
	depth(0.0f),
	radius(0.0f),
	life(-1),
	visible(true),
	infinite(false),
	foreground(false),
	background(false),
	hidden(false),
	trans(false),
	shadow(false),
	luminous(false),
	scene(nullptr)
{
	screen_rect.x = 0;
	screen_rect.y = 0;
	screen_rect.w = 0;
	screen_rect.h = 0;

	std::memset(name, 0, sizeof(name));

	strcpy_s(name, sizeof(name), "Graphic");
}

// +--------------------------------------------------------------------+

Graphic::~Graphic()
{
}

// +--------------------------------------------------------------------+

int
Graphic::operator < (const Graphic& g) const
{
	if (!infinite && g.infinite)
		return 1;

	if (infinite && !g.infinite)
		return 0;

	const double za = FMath::Abs((double)Depth());
	const double zb = FMath::Abs((double)g.Depth());

	return (za < zb) ? 1 : 0;
}

int
Graphic::operator <= (const Graphic& g) const
{
	if (!infinite && g.infinite)
		return 1;

	if (infinite && !g.infinite)
		return 0;

	const double za = FMath::Abs((double)Depth());
	const double zb = FMath::Abs((double)g.Depth());

	return (za <= zb) ? 1 : 0;
}

// +--------------------------------------------------------------------+

void
Graphic::SetInfinite(bool b)
{
	infinite = b;

	if (infinite)
		depth = 1.0e9f;
}

// +--------------------------------------------------------------------+

int
Graphic::Nearer(Graphic* a, Graphic* b)
{
	if (a->depth < b->depth) return -1;
	if (a->depth == b->depth) return 0;
	return 1;
}

// +--------------------------------------------------------------------+

int
Graphic::Farther(Graphic* a, Graphic* b)
{
	if (a->depth > b->depth) return -1;
	if (a->depth == b->depth) return 0;
	return 1;
}

// +--------------------------------------------------------------------+

void
Graphic::Destroy()
{
	if (scene)
		scene->DelGraphic(this);

	delete this;
}

// +--------------------------------------------------------------------+

int
Graphic::CollidesWith(Graphic& o)
{
	const FVector DeltaLoc = loc - o.loc;

	// bounding spheres test:
	if (DeltaLoc.Size() > radius + o.radius)
		return 0;

	return 1;
}

// +--------------------------------------------------------------------+

int
Graphic::CheckRayIntersection(FVector Q, FVector w, double len, FVector& ipt,
	bool treat_translucent_polys_as_solid)
{
	return 0;
}

// +--------------------------------------------------------------------+

void
Graphic::ProjectScreenRect(SimProjector* p)
{
	screen_rect.x = 2000;
	screen_rect.y = 2000;
	screen_rect.w = 0;
	screen_rect.h = 0;
}

// +--------------------------------------------------------------------+

bool
Graphic::CheckVisibility(SimProjector& projector)
{
	if (projector.IsVisible(Location(), Radius()) &&
		projector.ApparentRadius(Location(), Radius()) > 1)
	{
		visible = true;
	}
	else
	{
		visible = false;
		screen_rect.x = 2000;
		screen_rect.y = 2000;
		screen_rect.w = 0;
		screen_rect.h = 0;
	}

	return visible;
}
