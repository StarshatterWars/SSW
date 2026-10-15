#include "HulkVolnarisShipActor.h"
AHulkVolnarisShipActor::AHulkVolnarisShipActor()
{
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Volnaris"));
}
