#include "TankerAShipActor.h"

ATankerAShipActor::ATankerAShipActor()
{
    SetActorScale3D(FVector(2.5f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
