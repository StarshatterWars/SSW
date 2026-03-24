/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Asteroid.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Asteroid Sprite animation class
*/

#include "Asteroid.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "SimShot.h"
#include "Explosion.h"
#include "Sim.h"

#include "Solid.h"
#include "SimModel.h"
#include "DataLoader.h"
#include "Game.h"

// +--------------------------------------------------------------------+

static FVector asteroid_velocity = FVector::ZeroVector;
static SimModel* asteroid_model[32];

// +--------------------------------------------------------------------+

Asteroid::Asteroid(int t, const FVector& pos, double m)
	: Debris(asteroid_model[t % 6], pos, asteroid_velocity, m)
{
	life = -1;
}

// +--------------------------------------------------------------------+

void
Asteroid::Initialize()
{
	FMemory::Memzero(asteroid_model, sizeof(asteroid_model));

	DataLoader* loader = DataLoader::GetLoader();
	Text old_path = loader->GetDataPath();
	loader->SetDataPath("Galaxy/Asteroids/");

	int n = 0;

	SimModel* a = new SimModel;
	if (a) {
		a->Load("a1.mag", 100);
		asteroid_model[n++] = a;
	}

	a = new SimModel;
	if (a) {
		a->Load("a2.mag", 50);
		asteroid_model[n++] = a;
	}

	a = new SimModel;
	if (a) {
		a->Load("a1.mag", 8);
		asteroid_model[n++] = a;
	}

	a = new SimModel;
	if (a) {
		a->Load("a2.mag", 10);
		asteroid_model[n++] = a;
	}

	a = new SimModel;
	if (a) {
		a->Load("a3.mag", 30);
		asteroid_model[n++] = a;
	}

	a = new SimModel;
	if (a) {
		a->Load("a4.mag", 20);
		asteroid_model[n++] = a;
	}

	List<Text> mod_asteroids;
	loader->SetDataPath("Mods/Galaxy/Asteroids/");
	loader->ListFiles("*.mag", mod_asteroids);

	ListIter<Text> iter = mod_asteroids;
	while (++iter && n < 32) {
		a = new SimModel;
		if (a) {
			a->Load(*iter.value(), 50);
			asteroid_model[n++] = a;
		}
	}

	loader->SetDataPath(old_path);
}

void
Asteroid::Close()
{
	for (int i = 0; i < 32; i++)
		delete asteroid_model[i];

	FMemory::Memzero(asteroid_model, sizeof(asteroid_model));
}
