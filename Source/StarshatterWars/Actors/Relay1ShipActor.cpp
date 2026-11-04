#include "Relay1ShipActor.h"

ARelay1ShipActor::ARelay1ShipActor()
{
    SetActorScale3D(FVector(6.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
