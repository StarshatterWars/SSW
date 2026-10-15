#include "HulkImperialShipActor.h"
AHulkImperialShipActor::AHulkImperialShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Hulk-Imperial"));
    SetActorScale3D(FVector(4.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Imperial"));
}
