#include "FortressShipActor.h"
AFortressShipActor::AFortressShipActor()
{
    SetActorScale3D(FVector(11.0f));
    ConfigureTurretsFromShipDef(TEXT("Fortress"));
}
