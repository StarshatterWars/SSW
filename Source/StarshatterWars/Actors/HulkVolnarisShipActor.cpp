#include "HulkVolnarisShipActor.h"
AHulkVolnarisShipActor::AHulkVolnarisShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Volnaris"));
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Volnaris"));
}
