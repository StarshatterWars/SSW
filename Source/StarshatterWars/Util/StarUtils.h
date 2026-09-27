// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h" 
#include "Engine/TextureRenderTarget2D.h"

/**
 * 
 */
class STARSHATTERWARS_API StarUtils
{

public:
	/** Get default color for a given spectral class */
	static FLinearColor GetColor(ESPECTRAL_CLASS SpectralClass);

	/** Get emissive glow strength based on spectral class */
	static float GetGlowStrength(ESPECTRAL_CLASS SpectralClass);

	/** Optional: Get sunspot intensity */
	static float GetSunspotStrength(ESPECTRAL_CLASS SpectralClass);

	/** Format class as text label (e.g., "G-Class Star") */
	static FString GetSystemClassName(ESPECTRAL_CLASS SpectralClass);
	
	/** Stellar Rotation */
	static float GetRotationSpeed(float Rotation);

	/** Gets a UI image scale (in pixels) from radius using log scale */
	static float GetUISizeFromRadius(float Radius, float MinSize = 16.f, float MaxSize = 256.f);

	static UTextureRenderTarget2D* CreateStarRenderTarget(const FString& Name, UObject* Outer = nullptr, int32 Resolution = 256);

	// Set Astronomically-correct emmissive values
	static float GetEmissiveFromClass(ESPECTRAL_CLASS Class);

	// Estimate ideal render target resolution based on star radius in kilometers
	static int32 GetRenderTargetResolutionForRadius(double RadiusKm);

	UTexture2D* LoadStarAssetTexture(const FString& TextureName);
};
