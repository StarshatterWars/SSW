#include "HulkBroadswordShipActor.h"
AHulkBroadswordShipActor::AHulkBroadswordShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Broadsword"));
    SetActorScale3D(FVector(3.5f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Broadsword"));
}
