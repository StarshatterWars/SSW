#include "HulkDragonShipActor.h"
AHulkDragonShipActor::AHulkDragonShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Dragon"));
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Dragon"));
}
