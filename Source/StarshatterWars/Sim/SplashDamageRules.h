#pragma once
#include "CoreMinimal.h"
namespace SplashDamageRules
{
    inline bool Valid(double Damage,double Range,const FVector& Location)
    { return FMath::IsFinite(Damage) && FMath::IsFinite(Range) && !Location.ContainsNaN() && Damage>0 && Range>1; }
    inline double AtDistance(double Damage,double Range,double Distance)
    {
        return FMath::IsFinite(Damage) && FMath::IsFinite(Range) && FMath::IsFinite(Distance) &&
            Damage>0 && Range>1 && Distance>1 && Distance<Range ? Damage*(1-Distance/Range) : 0;
    }
}
