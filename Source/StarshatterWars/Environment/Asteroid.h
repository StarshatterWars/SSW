/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Asteroid.h
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Asteroid Sprite class
*/

#pragma once

#include "Types.h"
#include "Debris.h"

#include "Math/Vector.h"
#include "AsteroidResource.h"

// +--------------------------------------------------------------------+

class Asteroid : public Debris
{
public:
	Asteroid(int type, const FVector& pos, double mass);

    int GetMeshVariant() const {return MeshVariant;}
    const FSSWAsteroidResource& GetResource() const { return Resource; }
	static void Initialize();
	static void Close();
private:
    int MeshVariant=0;
    FSSWAsteroidResource Resource;
};

