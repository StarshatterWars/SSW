// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"
/**
 * 
 */
class STARSHATTERWARS_API PlanetOrbitUtils
{

public:
	static FVector GetPlanetOrbitPositionWithAngle(const FS_PlanetMap& PlanetData, const FVector& StarLocation, float OrbitAngleDegrees);

	// Returns a simple 2D orbit position (flat in X-Y) using a fixed orbit angle
	static FVector GetPlanetOrbitPositionFlat(const FS_PlanetMap& PlanetData, const FVector& StarLocation, float OrbitAngleDegrees);

	// 2D screen-space orbit position using radius (pre-scaled)
	static FVector2D Get2DOrbitPosition(float Radius, float OrbitAngleDegrees, float YTilt = 1.0f);

	// 2D screen-space orbit position using radius with inclination (pre-scaled)
	static FVector2D Get2DOrbitPositionWithInclination(float Radius, float OrbitAngleDegrees, float InclinationDegrees);

	// Limit orbit radius to fit within panel height based on inclination
	static float FitOrbitRadiusToPanel(float RawRadius, float InclinationDegrees, float PanelWidth, float PanelHeight, float Padding = 50.f);

	// Amplify inclination
	static float AmplifyInclination(float Deg, float Factor = 2.0f);

	/** Calculates perihelion and aphelion in kilometers */
	static void CalculateOrbitExtremes(float MeanOrbit, float Eccentricity, float& OutPerihelion, float& OutAphelion);
};
