#include "TigerShipActor.h"
ATigerShipActor::ATigerShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Tiger"));
    SetActorScale3D(FVector(1.8f));
    ConfigureTurretsFromShipDef(TEXT("Tiger"));
}
