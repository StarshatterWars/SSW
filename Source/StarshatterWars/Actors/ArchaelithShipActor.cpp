#include "ArchaelithShipActor.h"

AArchaelithShipActor::AArchaelithShipActor()
{
    SetActorScale3D(FVector(0.24f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
