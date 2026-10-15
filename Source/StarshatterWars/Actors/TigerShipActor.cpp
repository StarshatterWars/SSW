#include "TigerShipActor.h"
ATigerShipActor::ATigerShipActor()
{
    SetActorScale3D(FVector(1.8f));
    ConfigureTurretsFromShipDef(TEXT("Tiger"));
}
