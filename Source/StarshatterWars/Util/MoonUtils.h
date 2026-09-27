// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"


/**
 * 
 */
class STARSHATTERWARS_API MoonUtils
{
public:
	/** Gets a UI image scale (in pixels) from radius using log scale */
	static float GetUISizeFromRadius(float Radius, float MinSize = 16.f, float MaxSize = 64.f);

	/** Load uasset materials */
	static UTexture2D* LoadMoonAssetTexture(const FString& TextureName);

	/** Creates a unique 512x512 RGBA8 render target with black clear color */
	static UTextureRenderTarget2D* CreateMoonRenderTarget(const FString& BaseName, UObject* Outer = nullptr, int32 Resolution = 256);

	// Scale the UI based on planet radius in kilometers
	static float GetMoonUIScale(double RadiusKm); // in kilometers

	// planet rotation
	static FRotator GetMoonRotation(float TimeSeconds, float RotationSpeedDegreesPerSec, float TiltDegrees);

	static float GetNormalizedMoonUIScale(double RadiusKm);

	// tilt the planet
	static FRotator GetMoonAxisTilt(float TiltDegrees);

private:
	static TMap<FString, UTexture2D*> LoadedTextureCache;
};
