#pragma once
#include "NativeFlightPhysics.h"
class Physical;
// Game-thread adapter only: directors, RNG, live state commits and derived policies.
class FNativeFlightAdapter
{
public:
    static bool TryFrame(Physical&,ENativeFlightModel,double Seconds);
    static FNativeFlightState Capture(const Physical&);
private:
    static void Commit(Physical&,const FNativeFlightState&);
    static TArray<FVector> Samples(const FNativeFlightState&,double Seconds,double Subframe);
};
