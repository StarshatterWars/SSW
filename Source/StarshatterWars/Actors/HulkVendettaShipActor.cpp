#include "HulkVendettaShipActor.h"
AHulkVendettaShipActor::AHulkVendettaShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Vendetta"));
    SetActorScale3D(FVector(4.2f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Vendetta"));
}
