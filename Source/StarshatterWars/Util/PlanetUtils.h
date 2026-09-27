// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

/**
 * 
 */

class STARSHATTERWARS_API PlanetUtils
{
public:
	/** Loads Planet Texture */
	static UTexture2D* LoadPlanetTexture(const FString& TextureName);

	/** Creates a unique 512x512 RGBA8 render target with black clear color */
	static UTextureRenderTarget2D* CreatePlanetRenderTarget(const FString& BaseName, UObject* Outer = nullptr, int32 Resolution = 256);

	// tilt the planet
	static FRotator GetPlanetAxisTilt(float TiltDegrees);

	static float GetNormalizedPlanetUIScale(double RadiusKm);

private:
	static TMap<FString, UTexture2D*> LoadedTextureCache;


};
