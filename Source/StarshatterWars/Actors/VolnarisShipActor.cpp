#include "VolnarisShipActor.h"
AVolnarisShipActor::AVolnarisShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Volnaris"));
    SetActorScale3D(FVector(3.0f));
    ConfigureTurretsFromShipDef(TEXT("Volnaris"));
}
