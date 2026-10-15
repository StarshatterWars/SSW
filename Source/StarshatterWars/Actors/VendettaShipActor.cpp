#include "VendettaShipActor.h"
AVendettaShipActor::AVendettaShipActor()
{
    SetActorScale3D(FVector(4.2f));
    ConfigureTurretsFromShipDef(TEXT("Vendetta"));
}
