#include "DragonShipActor.h"
ADragonShipActor::ADragonShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Dragon"));
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Dragon"));
}
