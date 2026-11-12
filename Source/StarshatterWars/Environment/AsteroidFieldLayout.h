#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

template <typename Visitor>
void VisitAsteroidField(const TCHAR* RegionName, int32 Count, Visitor&& Visit)
{
    FRandomStream Random(FCrc::StrCrc32(RegionName));
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const double X = Random.FRandRange(-491520.0, 491520.0);
        const double Y = Random.FRandRange(-49152.0, 49152.0);
        const double Z = Random.FRandRange(-491520.0, 491520.0);
        const double Mass = Random.FRandRange(1e7, 1e8);
        Visit(FVector(X, Y, Z), Index, Mass);
    }
}
