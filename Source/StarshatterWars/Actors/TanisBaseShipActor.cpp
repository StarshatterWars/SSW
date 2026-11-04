#include "TanisBaseShipActor.h"

ATanisBaseShipActor::ATanisBaseShipActor()
{
    SetActorScale3D(FVector(4.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
