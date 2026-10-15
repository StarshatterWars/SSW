#include "ImperialShipActor.h"
AImperialShipActor::AImperialShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Imperial"));
    SetActorScale3D(FVector(4.0f));
    ConfigureTurretsFromShipDef(TEXT("Imperial"));
}
