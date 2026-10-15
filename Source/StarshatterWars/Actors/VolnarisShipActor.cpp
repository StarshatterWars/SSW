#include "VolnarisShipActor.h"
AVolnarisShipActor::AVolnarisShipActor()
{
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Volnaris"));
}
