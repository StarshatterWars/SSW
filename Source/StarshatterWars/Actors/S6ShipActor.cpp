#include "S6ShipActor.h"
AS6ShipActor::AS6ShipActor()
{
    SetActorScale3D(FVector(9.0f));
    ConfigureTurretsFromShipDef(TEXT("S6"));
}
