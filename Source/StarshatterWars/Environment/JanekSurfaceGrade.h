#pragma once
#include "CoreMinimal.h"

// Shared simulation/presentation datum for Janek's runway and parking apron.
namespace JanekSurfaceGrade
{
    constexpr double SoilHeight = -2.0;
    constexpr double HalfWidth = 12000.0;
    constexpr double EdgeFraction = 0.15;
    constexpr int32 RunwayPriority = -1000;
    constexpr int32 LakePriority = -900;
    inline double Apply(double NativeHeight, double Along, double Across)
    {
        const double Border=FMath::Min(HalfWidth-FMath::Abs(Along),HalfWidth-FMath::Abs(Across));
        const double Weight=FMath::SmoothStep(0.0,HalfWidth*2.0*EdgeFraction,Border);
        return FMath::Lerp(NativeHeight,SoilHeight,Weight);
    }
}
