#include "HulkDragonShipActor.h"
AHulkDragonShipActor::AHulkDragonShipActor()
{
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Dragon"));
}
