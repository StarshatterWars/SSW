#include "MarauderShipActor.h"
AMarauderShipActor::AMarauderShipActor()
{
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("Marauder"));
}
