#include "Mine1ShipActor.h"
AMine1ShipActor::AMine1ShipActor()
{
    SetActorScale3D(FVector(1.0f));
    ConfigureTurretsFromShipDef(TEXT("Mine1"));
}
