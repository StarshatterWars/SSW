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
#include "Game.h"
#include "NativeAsteroidActor.h"
#include "Engine/StaticMesh.h"

// +--------------------------------------------------------------------+

static FVector asteroid_velocity = FVector::ZeroVector;
static double library_radii[6]{};

// +--------------------------------------------------------------------+

Asteroid::Asteroid(int t, const FVector& pos, double m)
	: Debris(nullptr, pos, asteroid_velocity, m)
{
    MeshVariant=((t%6)+6)%6;
    // Heavy debris spins at a barely visible 0.14 degrees per second.
    // Seed a sustained, distinct angular velocity for each rock in the field.
    FRandomStream Spin(t);
    const double Speed = Spin.FRandRange(0.14, 0.28);
    SetAngularVelocity(Spin.VRand() * Speed);
    // Only the imported library supplies asteroid geometry and raw bounds.
    radius=library_radii[MeshVariant];
	life = -1;
}

// +--------------------------------------------------------------------+

void
Asteroid::Initialize()
{
    FMemory::Memzero(library_radii,sizeof(library_radii));
    const auto* Library=GetDefault<UAsteroidLibrarySettings>();
    for(int I=0;I<6 && Library->Meshes.Num()>0;++I)
        if(auto* Mesh=Library->Meshes[I%Library->Meshes.Num()].LoadSynchronous())library_radii[I]=ANativeAsteroidActor::RawRadius(Mesh->GetBounds().SphereRadius);
    // No MAG model loading: ANativeAsteroidActor renders the configured pack.
    // Debris retains simulation damage and spherical collision using raw radii.
}

void
Asteroid::Close()
{
    FMemory::Memzero(library_radii,sizeof(library_radii));
}
