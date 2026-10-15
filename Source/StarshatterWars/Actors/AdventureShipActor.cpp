#include "AdventureShipActor.h"
AAdventureShipActor::AAdventureShipActor()
{
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("Adventure"));
}
