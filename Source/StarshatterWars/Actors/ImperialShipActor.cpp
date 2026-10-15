#include "ImperialShipActor.h"
AImperialShipActor::AImperialShipActor()
{
    SetActorScale3D(FVector(4.0f));
    ConfigureTurretsFromShipDef(TEXT("Imperial"));
}
