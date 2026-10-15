#include "HulkTigerShipActor.h"
AHulkTigerShipActor::AHulkTigerShipActor()
{
    SetActorScale3D(FVector(1.8f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Tiger"));
}
