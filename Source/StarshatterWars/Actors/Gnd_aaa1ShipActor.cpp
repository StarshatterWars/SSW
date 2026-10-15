#include "Gnd_aaa1ShipActor.h"
AGnd_aaa1ShipActor::AGnd_aaa1ShipActor()
{
    SetActorScale3D(FVector(2.0f));
    ConfigureTurretsFromShipDef(TEXT("Gnd_aaa1"));
}
