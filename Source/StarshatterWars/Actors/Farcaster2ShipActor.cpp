#include "Farcaster2ShipActor.h"

AFarcaster2ShipActor::AFarcaster2ShipActor()
{
    SetActorScale3D(FVector(20.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
