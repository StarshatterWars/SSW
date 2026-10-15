#include "HulkImperialShipActor.h"
AHulkImperialShipActor::AHulkImperialShipActor()
{
    SetActorScale3D(FVector(4.0f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Imperial"));
}
