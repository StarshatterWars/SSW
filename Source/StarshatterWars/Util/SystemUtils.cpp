/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         SystemUtils.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Shared astronomical and system-scene utility helpers.
*/

#include "SystemUtils.h"

double USystemUtils::KilometersToAU(double Kilometers)
{
    return Kilometers / KmPerAU;
}

double USystemUtils::AUToKilometers(double AU)
{
    return AU * KmPerAU;
}

double USystemUtils::KilometersToLightSeconds(double Kilometers)
{
    return Kilometers / KmPerLightSecond;
}

double USystemUtils::LightSecondsToKilometers(double LightSeconds)
{
    return LightSeconds * KmPerLightSecond;
}

double USystemUtils::KilometersToLightMinutes(double Kilometers)
{
    return Kilometers / KmPerLightMinute;
}

double USystemUtils::LightMinutesToKilometers(double LightMinutes)
{
    return LightMinutes * KmPerLightMinute;
}

double USystemUtils::KilometersToLightYears(double Kilometers)
{
    return Kilometers / KmPerLightYear;
}

double USystemUtils::LightYearsToKilometers(double LightYears)
{
    return LightYears * KmPerLightYear;
}

float USystemUtils::OrbitKmToSceneUnits(float OrbitKm, float UnitsPerMillionKm)
{
    const float MillionKm = OrbitKm / 1000000.0f;
    return MillionKm * UnitsPerMillionKm;
}

float USystemUtils::RadiusKmToSceneUnits(float RadiusKm, float UnitsPerThousandKm)
{
    const float ThousandKm = RadiusKm / 1000.0f;
    return ThousandKm * UnitsPerThousandKm;
}

float USystemUtils::DiameterKmToSceneUnits(float DiameterKm, float UnitsPerThousandKm)
{
    const float ThousandKm = DiameterKm / 1000.0f;
    return ThousandKm * UnitsPerThousandKm;
}

float USystemUtils::SceneUnitsToOrbitKm(float SceneUnits, float UnitsPerMillionKm)
{
    if (FMath::IsNearlyZero(UnitsPerMillionKm))
    {
        return 0.0f;
    }

    return (SceneUnits / UnitsPerMillionKm) * 1000000.0f;
}

float USystemUtils::OrbitKmToSceneUnitsClamped(
    float OrbitKm,
    float UnitsPerMillionKm,
    float MinSceneUnits,
    float MaxSceneUnits)
{
    const float Raw = OrbitKmToSceneUnits(OrbitKm, UnitsPerMillionKm);
    return FMath::Clamp(Raw, MinSceneUnits, MaxSceneUnits);
}

float USystemUtils::OrbitKmToSceneUnitsLog(
    float OrbitKm,
    float MinOrbitKm,
    float MaxOrbitKm,
    float MinSceneUnits,
    float MaxSceneUnits)
{
    const float SafeOrbit = FMath::Max(OrbitKm, 1.0f);
    const float SafeMin = FMath::Max(MinOrbitKm, 1.0f);
    const float SafeMax = FMath::Max(MaxOrbitKm, SafeMin + 1.0f);

    const float LogValue = FMath::LogX(10.0f, SafeOrbit);
    const float LogMin = FMath::LogX(10.0f, SafeMin);
    const float LogMax = FMath::LogX(10.0f, SafeMax);

    return RemapRangeClamped(LogValue, LogMin, LogMax, MinSceneUnits, MaxSceneUnits);
}

float USystemUtils::RadiusKmToSceneUnitsClamped(
    float RadiusKm,
    float UnitsPerThousandKm,
    float MinSceneUnits,
    float MaxSceneUnits)
{
    const float Raw = RadiusKmToSceneUnits(RadiusKm, UnitsPerThousandKm);
    return FMath::Clamp(Raw, MinSceneUnits, MaxSceneUnits);
}

FVector USystemUtils::MakeOrbitPosition2D(float OrbitRadiusUnits, float AngleDegrees, float Z)
{
    const float Radians = FMath::DegreesToRadians(AngleDegrees);
    const float X = OrbitRadiusUnits * FMath::Cos(Radians);
    const float Y = OrbitRadiusUnits * FMath::Sin(Radians);

    return FVector(X, Y, Z);
}

FVector USystemUtils::MakeOrbitPosition3D(
    float OrbitRadiusUnits,
    float AngleDegrees,
    float InclinationDegrees,
    float AscendingNodeDegrees)
{
    const float OrbitRadians = FMath::DegreesToRadians(AngleDegrees);
    const float InclinationRadians = FMath::DegreesToRadians(InclinationDegrees);
    const float NodeRadians = FMath::DegreesToRadians(AscendingNodeDegrees);

    const FVector FlatPos(
        OrbitRadiusUnits * FMath::Cos(OrbitRadians),
        OrbitRadiusUnits * FMath::Sin(OrbitRadians),
        0.0f);

    const FRotator InclinationRot(0.0f, 0.0f, InclinationDegrees);
    const FRotator NodeRot(0.0f, AscendingNodeDegrees, 0.0f);

    FVector Result = InclinationRot.RotateVector(FlatPos);
    Result = NodeRot.RotateVector(Result);

    return Result;
}

float USystemUtils::NormalizeDegrees(float Degrees)
{
    float Result = FMath::Fmod(Degrees, 360.0f);
    if (Result < 0.0f)
    {
        Result += 360.0f;
    }
    return Result;
}

float USystemUtils::DegreesToRadiansFloat(float Degrees)
{
    return FMath::DegreesToRadians(Degrees);
}

float USystemUtils::RadiansToDegreesFloat(float Radians)
{
    return FMath::RadiansToDegrees(Radians);
}

float USystemUtils::OrbitFractionToDegrees(float OrbitFraction)
{
    return NormalizeDegrees(OrbitFraction * 360.0f);
}

float USystemUtils::TimeToOrbitDegrees(float ElapsedSeconds, float OrbitPeriodSeconds, float StartDegrees)
{
    if (OrbitPeriodSeconds <= KINDA_SMALL_NUMBER)
    {
        return NormalizeDegrees(StartDegrees);
    }

    const float Fraction = ElapsedSeconds / OrbitPeriodSeconds;
    return NormalizeDegrees(StartDegrees + (Fraction * 360.0f));
}

float USystemUtils::TimeToRotationDegrees(float ElapsedSeconds, float RotationPeriodSeconds, float StartDegrees)
{
    if (RotationPeriodSeconds <= KINDA_SMALL_NUMBER)
    {
        return NormalizeDegrees(StartDegrees);
    }

    const float Fraction = ElapsedSeconds / RotationPeriodSeconds;
    return NormalizeDegrees(StartDegrees + (Fraction * 360.0f));
}

double USystemUtils::SurfaceGravityMps2(double MassKg, double RadiusMeters)
{
    if (MassKg <= 0.0 || RadiusMeters <= 0.0)
    {
        return 0.0;
    }

    return (GravitationalConstant * MassKg) / (RadiusMeters * RadiusMeters);
}

double USystemUtils::EscapeVelocityMps(double MassKg, double RadiusMeters)
{
    if (MassKg <= 0.0 || RadiusMeters <= 0.0)
    {
        return 0.0;
    }

    return FMath::Sqrt((2.0 * GravitationalConstant * MassKg) / RadiusMeters);
}

double USystemUtils::CircularOrbitVelocityMps(double CentralMassKg, double OrbitRadiusMeters)
{
    if (CentralMassKg <= 0.0 || OrbitRadiusMeters <= 0.0)
    {
        return 0.0;
    }

    return FMath::Sqrt((GravitationalConstant * CentralMassKg) / OrbitRadiusMeters);
}

FString USystemUtils::FormatDistanceKm(double Kilometers)
{
    if (Kilometers >= 1000000000.0)
    {
        return FString::Printf(TEXT("%.2f billion km"), Kilometers / 1000000000.0);
    }

    if (Kilometers >= 1000000.0)
    {
        return FString::Printf(TEXT("%.2f million km"), Kilometers / 1000000.0);
    }

    if (Kilometers >= 1000.0)
    {
        return FString::Printf(TEXT("%.0f km"), Kilometers);
    }

    return FString::Printf(TEXT("%.2f km"), Kilometers);
}

FString USystemUtils::FormatDistanceAU(double Kilometers, int32 FractionalDigits)
{
    const double AU = KilometersToAU(Kilometers);
    return FString::Printf(TEXT("%.*f AU"), FractionalDigits, AU);
}

FString USystemUtils::FormatDistanceLightSeconds(double Kilometers, int32 FractionalDigits)
{
    const double LS = KilometersToLightSeconds(Kilometers);
    return FString::Printf(TEXT("%.*f ls"), FractionalDigits, LS);
}

FString USystemUtils::FormatDistanceAdaptive(double Kilometers)
{
    if (Kilometers >= KmPerLightYear * 0.01)
    {
        return FString::Printf(TEXT("%.3f ly"), KilometersToLightYears(Kilometers));
    }

    if (Kilometers >= KmPerAU * 0.01)
    {
        return FString::Printf(TEXT("%.3f AU"), KilometersToAU(Kilometers));
    }

    if (Kilometers >= KmPerLightSecond)
    {
        return FString::Printf(TEXT("%.2f ls"), KilometersToLightSeconds(Kilometers));
    }

    return FormatDistanceKm(Kilometers);
}

float USystemUtils::SafeDivide(float Numerator, float Denominator, float Fallback)
{
    if (FMath::IsNearlyZero(Denominator))
    {
        return Fallback;
    }

    return Numerator / Denominator;
}

double USystemUtils::SafeDivideDouble(double Numerator, double Denominator, double Fallback)
{
    if (FMath::IsNearlyZero((float)Denominator))
    {
        return Fallback;
    }

    return Numerator / Denominator;
}

float USystemUtils::RemapRangeClamped(float Value, float InMin, float InMax, float OutMin, float OutMax)
{
    if (FMath::IsNearlyEqual(InMin, InMax))
    {
        return OutMin;
    }

    const float Alpha = FMath::Clamp((Value - InMin) / (InMax - InMin), 0.0f, 1.0f);
    return FMath::Lerp(OutMin, OutMax, Alpha);
}