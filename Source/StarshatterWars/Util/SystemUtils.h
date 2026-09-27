/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         SystemUtils.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Shared astronomical and system-scene utility helpers.

    PURPOSE
    -------
    Centralizes common conversions and calculations used by:
    - system scene generation
    - system map rendering
    - orbit ring drawing
    - planet/moon placement
    - gameplay distance scaling

    NOTES
    -----
    These helpers intentionally separate:
    - real astronomical units
    - gameplay/system-data units
    - Unreal scene units

    This prevents hardcoded per-screen math from drifting over time.
*/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SystemUtils.generated.h"

UCLASS()
class STARSHATTERWARS_API USystemUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ---------------------------------------------------------------------
    // Physical constants
    // ---------------------------------------------------------------------

    static constexpr double KmPerAU = 149597870.7;
    static constexpr double KmPerLightSecond = 299792.458;
    static constexpr double KmPerLightMinute = 17987547.48;
    static constexpr double KmPerLightYear = 9460730472580.8;
    static constexpr double SecondsPerDay = 86400.0;
    static constexpr double SecondsPerHour = 3600.0;
    static constexpr double DegreesPerCircle = 360.0;
    static constexpr double RadiansPerCircle = 2.0 * PI;

public:
    // ---------------------------------------------------------------------
    // Basic distance conversions
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double KilometersToAU(double Kilometers);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double AUToKilometers(double AU);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double KilometersToLightSeconds(double Kilometers);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double LightSecondsToKilometers(double LightSeconds);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double KilometersToLightMinutes(double Kilometers);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double LightMinutesToKilometers(double LightMinutes);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double KilometersToLightYears(double Kilometers);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double LightYearsToKilometers(double LightYears);

public:
    // ---------------------------------------------------------------------
    // Unreal scene-space conversions
    // ---------------------------------------------------------------------
    // These use tunable scale values supplied by the caller so the same
    // helpers can support:
    // - cinematic scene maps
    // - UI system views
    // - overview representations
    // - close-up tactical system scenes

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float OrbitKmToSceneUnits(float OrbitKm, float UnitsPerMillionKm = 100.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float RadiusKmToSceneUnits(float RadiusKm, float UnitsPerThousandKm = 10.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float DiameterKmToSceneUnits(float DiameterKm, float UnitsPerThousandKm = 10.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float SceneUnitsToOrbitKm(float SceneUnits, float UnitsPerMillionKm = 100.0f);

public:
    // ---------------------------------------------------------------------
    // Clamped/log-scaled scene helpers
    // Useful when real orbit ranges are too large to display linearly.
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float OrbitKmToSceneUnitsClamped(
        float OrbitKm,
        float UnitsPerMillionKm,
        float MinSceneUnits,
        float MaxSceneUnits);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float OrbitKmToSceneUnitsLog(
        float OrbitKm,
        float MinOrbitKm,
        float MaxOrbitKm,
        float MinSceneUnits,
        float MaxSceneUnits);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float RadiusKmToSceneUnitsClamped(
        float RadiusKm,
        float UnitsPerThousandKm,
        float MinSceneUnits,
        float MaxSceneUnits);

public:
    // ---------------------------------------------------------------------
    // Orbit helpers
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FVector MakeOrbitPosition2D(float OrbitRadiusUnits, float AngleDegrees, float Z = 0.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FVector MakeOrbitPosition3D(
        float OrbitRadiusUnits,
        float AngleDegrees,
        float InclinationDegrees,
        float AscendingNodeDegrees = 0.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float NormalizeDegrees(float Degrees);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float DegreesToRadiansFloat(float Degrees);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float RadiansToDegreesFloat(float Radians);

public:
    // ---------------------------------------------------------------------
    // Orbital timing helpers
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float OrbitFractionToDegrees(float OrbitFraction);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float TimeToOrbitDegrees(float ElapsedSeconds, float OrbitPeriodSeconds, float StartDegrees = 0.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float TimeToRotationDegrees(float ElapsedSeconds, float RotationPeriodSeconds, float StartDegrees = 0.0f);

public:
    // ---------------------------------------------------------------------
    // Gravity / escape velocity / orbital speed
    // Approximate but useful for gameplay and reference displays.
    // Inputs:
    // - MassKg in kilograms
    // - RadiusMeters / OrbitRadiusMeters in meters
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double SurfaceGravityMps2(double MassKg, double RadiusMeters);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double EscapeVelocityMps(double MassKg, double RadiusMeters);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double CircularOrbitVelocityMps(double CentralMassKg, double OrbitRadiusMeters);

public:
    // ---------------------------------------------------------------------
    // Formatting helpers
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FString FormatDistanceKm(double Kilometers);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FString FormatDistanceAU(double Kilometers, int32 FractionalDigits = 2);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FString FormatDistanceLightSeconds(double Kilometers, int32 FractionalDigits = 2);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static FString FormatDistanceAdaptive(double Kilometers);

public:
    // ---------------------------------------------------------------------
    // Generic math helpers
    // ---------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float SafeDivide(float Numerator, float Denominator, float Fallback = 0.0f);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static double SafeDivideDouble(double Numerator, double Denominator, double Fallback = 0.0);

    UFUNCTION(BlueprintPure, Category = "Starshatter|SystemUtils")
    static float RemapRangeClamped(float Value, float InMin, float InMax, float OutMin, float OutMax);

private:
    static constexpr double GravitationalConstant = 6.67430e-11;
};