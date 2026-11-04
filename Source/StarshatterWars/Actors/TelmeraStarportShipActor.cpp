#include "TelmeraStarportShipActor.h"

ATelmeraStarportShipActor::ATelmeraStarportShipActor()
{
    SetActorScale3D(FVector(4.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
