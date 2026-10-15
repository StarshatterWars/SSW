#include "HulkTigerShipActor.h"
AHulkTigerShipActor::AHulkTigerShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Tiger"));
    SetActorScale3D(FVector(1.8f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Tiger"));
}
