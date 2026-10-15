#include "Gnd_sam1ShipActor.h"
AGnd_sam1ShipActor::AGnd_sam1ShipActor()
{
    SetActorScale3D(FVector(2.0f));
    ConfigureTurretsFromShipDef(TEXT("Gnd_sam1"));
}
