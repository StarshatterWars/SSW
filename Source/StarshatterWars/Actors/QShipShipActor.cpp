#include "QShipShipActor.h"
AQShipShipActor::AQShipShipActor()
{
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("QShip"));
}
