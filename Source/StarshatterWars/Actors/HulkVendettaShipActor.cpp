#include "HulkVendettaShipActor.h"
AHulkVendettaShipActor::AHulkVendettaShipActor()
{
    SetActorScale3D(FVector(4.2f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Vendetta"));
}
