#pragma once
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

// Legacy hull multiplier with the requested 100-metre minimum. Simulation metres.
namespace LegacyWaypointArrival
{
    constexpr double HullRadiusMultiplier = 2.0;
    constexpr double MinimumArrivalRadius = 100.0;
    inline double Radius(double HullRadius) { return FMath::Max(MinimumArrivalRadius, HullRadiusMultiplier * FMath::Max(0.0, HullRadius)); }
    inline bool Reached(const FVector& Current, const FVector& Previous,
                        const FVector& Destination, double ArrivalRadius, bool HasPrevious)
    {
        if (ArrivalRadius <= 0.0 || Current.ContainsNaN() || Destination.ContainsNaN()) return false;
        const double RadiusSquared = ArrivalRadius * ArrivalRadius;
        if ((Current-Destination).SizeSquared() < RadiusSquared) return true;
        if (!HasPrevious || Previous.ContainsNaN()) return false;
        const FVector Segment = Current-Previous;
        const double LengthSquared = Segment.SizeSquared();
        if (LengthSquared <= 1.e-12) return false;
        const double Fraction = FMath::Clamp(FVector::DotProduct(Destination-Previous, Segment)/LengthSquared, 0.0, 1.0);
        return (Previous + Segment*Fraction-Destination).SizeSquared() < RadiusSquared;
    }
}
