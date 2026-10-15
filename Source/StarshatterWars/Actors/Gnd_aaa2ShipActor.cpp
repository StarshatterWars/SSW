#include "Gnd_aaa2ShipActor.h"
AGnd_aaa2ShipActor::AGnd_aaa2ShipActor()
{
    SetActorScale3D(FVector(2.0f));
    ConfigureTurretsFromShipDef(TEXT("Gnd_aaa2"));
}
