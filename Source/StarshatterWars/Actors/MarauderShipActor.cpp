#include "MarauderShipActor.h"
AMarauderShipActor::AMarauderShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Marauder"));
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("Marauder"));
}
