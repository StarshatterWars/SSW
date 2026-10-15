#include "VendettaShipActor.h"
AVendettaShipActor::AVendettaShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Vendetta"));
    SetActorScale3D(FVector(4.2f));
    ConfigureTurretsFromShipDef(TEXT("Vendetta"));
}
