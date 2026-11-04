#include "FreightXferShipActor.h"

AFreightXferShipActor::AFreightXferShipActor()
{
    SetActorScale3D(FVector(5.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
