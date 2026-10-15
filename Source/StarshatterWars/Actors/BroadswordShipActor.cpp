#include "BroadswordShipActor.h"
ABroadswordShipActor::ABroadswordShipActor()
{
    SetActorScale3D(FVector(3.5f));
    ConfigureTurretsFromShipDef(TEXT("Broadsword"));
}
