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
#include "Misc/Paths.h"
#include "NativeAsteroidActor.h"
#include "Engine/StaticMesh.h"

// +--------------------------------------------------------------------+

static FVector asteroid_velocity = FVector::ZeroVector;
static SimModel* asteroid_model[32];
static double library_radii[6]{};

// +--------------------------------------------------------------------+

Asteroid::Asteroid(int t, const FVector& pos, double m)
	: Debris(asteroid_model[((t % 6)+6)%6], pos, asteroid_velocity, m)
{
    MeshVariant=((t%6)+6)%6;
    if(library_radii[MeshVariant]>0){radius=library_radii[MeshVariant];bLibraryBounds=true;}
	life = -1;
}

// +--------------------------------------------------------------------+

int Asteroid::HitBy(SimShot* Shot,FVector& Impact)
{
    if(!bLibraryBounds)return Debris::HitBy(Shot,Impact);
    // The imported mesh replaces the old MAG visual. Use its raw bounding
    // radius for simulation collision, not the unrelated old mesh triangles.
    Graphic* LegacyRep=rep;rep=nullptr;
    const int Hit=Debris::HitBy(Shot,Impact);
    rep=LegacyRep;return Hit;
}

void
Asteroid::Initialize()
{
    FMemory::Memzero(library_radii,sizeof(library_radii));
    const auto* Library=GetDefault<UAsteroidLibrarySettings>();
    for(int I=0;I<6 && Library->Meshes.Num()>0;++I)
        if(auto* Mesh=Library->Meshes[I%Library->Meshes.Num()].LoadSynchronous())library_radii[I]=ANativeAsteroidActor::RawRadius(Mesh->GetBounds().SphereRadius);
	FMemory::Memzero(asteroid_model, sizeof(asteroid_model));

	DataLoader* loader = DataLoader::GetLoader();
	Text old_path = loader->GetDataPath();
	loader->SetDataPath(TCHAR_TO_ANSI(*(FPaths::ProjectContentDir()/TEXT("GameData/Galaxy/Asteroids/"))));

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

	loader->SetDataPath(old_path);
}

void
Asteroid::Close()
{
	for (int i = 0; i < 32; i++)
		delete asteroid_model[i];

	FMemory::Memzero(asteroid_model, sizeof(asteroid_model));
}
