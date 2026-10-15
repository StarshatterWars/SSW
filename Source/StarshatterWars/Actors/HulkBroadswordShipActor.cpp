#include "HulkBroadswordShipActor.h"
AHulkBroadswordShipActor::AHulkBroadswordShipActor()
{
    SetActorScale3D(FVector(3.5f));
    ConfigureTurretsFromShipDef(TEXT("Hulk-Broadsword"));
}
