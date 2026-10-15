#include "QShipShipActor.h"
AQShipShipActor::AQShipShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("QShip"));
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("QShip"));
}
