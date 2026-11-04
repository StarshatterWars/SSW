#include "ColonyShipActor.h"

AColonyShipActor::AColonyShipActor()
{
    SetActorScale3D(FVector(7.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
