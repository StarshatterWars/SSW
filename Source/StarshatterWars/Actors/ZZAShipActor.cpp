#include "ZZAShipActor.h"
AZZAShipActor::AZZAShipActor()
{
    SetActorScale3D(FVector(0.75f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZA"));
}
