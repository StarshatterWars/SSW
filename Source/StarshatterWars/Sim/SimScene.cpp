/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         SimScene.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL:     John DiCamillo / Destroyer Studios LLC

	OVERVIEW
	========
	A 3D Scene
*/

#include "SimScene.h"
#include "Graphic.h"
#include "SimLight.h"

// Minimal Unreal includes:
#include "Math/Vector.h"
#include "Logging/LogMacros.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterSimScene, Log, All);

// +--------------------------------------------------------------------+

SimScene::SimScene()
{
}

SimScene::~SimScene()
{
	background.destroy();
	foreground.destroy();
	graphics.destroy();
	sprites.destroy();
	lights.destroy();
}

// +--------------------------------------------------------------------+

void
SimScene::AddBackground(Graphic* g)
{
	if (g) {
		if (!background.contains(g))
			background.append(g);

		g->SetScene(this);
	}
}

void
SimScene::DelBackground(Graphic* g)
{
	if (g) {
		background.remove(g);
		g->SetScene(0);
	}
}

// +--------------------------------------------------------------------+

void
SimScene::AddForeground(Graphic* g)
{
	if (g) {
		if (!foreground.contains(g))
			foreground.append(g);

		g->SetScene(this);
	}
}

void
SimScene::DelForeground(Graphic* g)
{
	if (g) {
		foreground.remove(g);
		g->SetScene(0);
	}
}

// +--------------------------------------------------------------------+

void
SimScene::AddGraphic(Graphic* g)
{
	if (g) {
		if (!graphics.contains(g))
			graphics.append(g);

		g->SetScene(this);
	}
}

void
SimScene::DelGraphic(Graphic* g)
{
	if (g) {
		graphics.remove(g) || // it's gotta be in here somewhere!
			foreground.remove(g) || // use the logical-or operator to early
			sprites.remove(g) || // out when we find it...
			background.remove(g);

		g->SetScene(0);
	}
}

// +--------------------------------------------------------------------+

void
SimScene::AddSprite(Graphic* g)
{
	if (g) {
		if (!sprites.contains(g))
			sprites.append(g);

		g->SetScene(this);
	}
}

void
SimScene::DelSprite(Graphic* g)
{
	if (g) {
		sprites.remove(g);
		g->SetScene(0);
	}
}

// +--------------------------------------------------------------------+

void
SimScene::AddLight(SimLight* l)
{
	if (l) {
		if (!lights.contains(l))
			lights.append(l);

		l->SetScene(this);
	}
}

void
SimScene::DelLight(SimLight* l)
{
	if (l) {
		lights.remove(l);
		l->SetScene(0);
	}
}

// +--------------------------------------------------------------------+

void
SimScene::Collect()
{
	ListIter<Graphic> iter = graphics;

	while (++iter) {
		Graphic* g = iter.value();
		if (g->Life() == 0) {
			delete iter.removeItem();
		}
	}

	iter.attach(sprites);

	while (++iter) {
		Graphic* g = iter.value();
		if (g->Life() == 0) {
			delete iter.removeItem();
		}
	}

	ListIter<SimLight> iter1 = lights;

	while (++iter1) {
		SimLight* l = iter1.value();
		if (l->Life() == 0) {
			delete iter1.removeItem();
		}
	}
}

// +--------------------------------------------------------------------+

bool SimScene::IsLightObscured(
	UWorld* World,
	const FVector& obj_pos,
	const FVector& light_pos,
	double obj_radius,
	FVector* impact_point) const
{
	if (!World)
		return false;

	const FVector Dir = (light_pos - obj_pos);
	const double Len = Dir.Length();

	if (Len <= KINDA_SMALL_NUMBER)
		return false;

	FHitResult Hit;
	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(SimScene_LightObscured),
		/*bTraceComplex=*/true
	);

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		obj_pos,
		light_pos,
		ECC_Visibility,
		Params
	);

	if (bHit)
	{
		if (impact_point)
			*impact_point = Hit.ImpactPoint;
		return true;
	}

	return false;
}

bool SimScene::IsLightObscured(
	const FVector& obj_pos,
	const FVector& light_pos,
	double obj_radius,
	FVector* impact_point) const
{
	// Legacy bridge: try to obtain a valid UWorld automatically.
	UWorld* World = nullptr;

	if (GEngine && GEngine->GameViewport)
		World = GEngine->GameViewport->GetWorld();

	// If no world is available (early init, tools, etc.), return false safely.
	return IsLightObscured(World, obj_pos, light_pos, obj_radius, impact_point);
}