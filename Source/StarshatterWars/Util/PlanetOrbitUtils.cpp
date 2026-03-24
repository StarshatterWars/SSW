// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "PlanetOrbitUtils.h"

FVector PlanetOrbitUtils::GetPlanetOrbitPositionWithAngle(const FS_PlanetMap& PlanetData, const FVector& StarLocation, float OrbitAngleDegrees)
{
	const float OrbitRadiusCM = PlanetData.Orbit * 100.0f;
	const float OrbitAngleRad = FMath::DegreesToRadians(OrbitAngleDegrees);
	const float InclinationDegrees = PlanetData.Inclination;

	FVector FlatOrbit = FVector(
		FMath::Cos(OrbitAngleRad) * OrbitRadiusCM,
		FMath::Sin(OrbitAngleRad) * OrbitRadiusCM,
		0.f
	);

	FVector TiltedOrbit = FlatOrbit.RotateAngleAxis(InclinationDegrees, FVector::RightVector);
	return StarLocation + TiltedOrbit;
}

FVector PlanetOrbitUtils::GetPlanetOrbitPositionFlat(const FS_PlanetMap& PlanetData, const FVector& StarLocation, float OrbitAngleDegrees)
{
	const float OrbitRadiusCM = PlanetData.Orbit * 100.0f; // km to cm
	const float OrbitAngleRad = FMath::DegreesToRadians(OrbitAngleDegrees);

	// Flat orbit in X-Y plane
	FVector OrbitPosition = FVector(
		FMath::Cos(OrbitAngleRad) * OrbitRadiusCM,
		FMath::Sin(OrbitAngleRad) * OrbitRadiusCM,
		0.f // flat
	);

	return StarLocation + OrbitPosition;
}

FVector2D PlanetOrbitUtils::Get2DOrbitPosition(float Radius, float OrbitAngleDegrees, float YTilt)
{
	const float AngleRad = FMath::DegreesToRadians(OrbitAngleDegrees);

	return FVector2D(
		FMath::Cos(AngleRad) * Radius,
		FMath::Sin(AngleRad) * Radius * YTilt
	);
}

FVector2D PlanetOrbitUtils::Get2DOrbitPositionWithInclination(float Radius, float OrbitAngleDegrees, float InclinationDegrees)
{
	const float AngleRad = FMath::DegreesToRadians(OrbitAngleDegrees);
	const float InclinationRad = FMath::DegreesToRadians(InclinationDegrees);

	// Compute unrotated orbit point (in X-Y plane)
	float X = FMath::Cos(AngleRad) * Radius;
	float Y = FMath::Sin(AngleRad) * Radius;

	// Apply inclination by rotating around X axis (affects Y visually)
	// In 2D, we simulate this by squashing Y based on inclination angle
	Y *= FMath::Cos(InclinationRad);

	return FVector2D(X, Y);
}

float PlanetOrbitUtils::FitOrbitRadiusToPanel(float RawRadius, float InclinationDegrees, float PanelWidth, float PanelHeight, float Padding)
{
	const float InclinationRad = FMath::DegreesToRadians(InclinationDegrees);
	const float YTilt = FMath::Cos(InclinationRad);

	// Compute max radius allowed based on vertical and horizontal bounds
	const float MaxVerticalRadius = (PanelHeight * 0.5f - Padding) / (YTilt > 0.001f ? YTilt : 1.0f);
	const float MaxHorizontalRadius = (PanelWidth * 0.5f) - Padding;

	// Final radius must satisfy both constraints
	const float MaxRadius = FMath::Min(MaxVerticalRadius, MaxHorizontalRadius);

	return FMath::Min(RawRadius, MaxRadius);
}

float PlanetOrbitUtils::AmplifyInclination(float Deg, float Factor)
{
	return Deg * Factor;
}

void PlanetOrbitUtils::CalculateOrbitExtremes(float MeanOrbit, float Eccentricity, float& OutPerihelion, float& OutAphelion)
{
	OutPerihelion = MeanOrbit * (1.0f - Eccentricity);
	OutAphelion = MeanOrbit * (1.0f + Eccentricity);
}