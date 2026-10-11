#pragma once
#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"

// Stars45/NavAI.cpp: distance < 2 * self->Radius(). Simulation metres.
namespace LegacyWaypointArrival
{
    constexpr double HullRadiusMultiplier = 2.0;
    inline double Radius(double HullRadius) { return HullRadiusMultiplier * FMath::Max(0.0, HullRadius); }
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
