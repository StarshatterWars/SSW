#include "S5ShipActor.h"
AS5ShipActor::AS5ShipActor()
{
    SetActorScale3D(FVector(8.0f));
    ConfigureTurretsFromShipDef(TEXT("S5"));
}
