#include "S7ShipActor.h"
AS7ShipActor::AS7ShipActor()
{
    SetActorScale3D(FVector(15.0f));
    ConfigureTurretsFromShipDef(TEXT("S7"));
}
