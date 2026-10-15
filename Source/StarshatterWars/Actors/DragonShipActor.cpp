#include "DragonShipActor.h"
ADragonShipActor::ADragonShipActor()
{
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Dragon"));
}
