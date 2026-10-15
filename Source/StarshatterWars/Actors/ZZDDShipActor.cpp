#include "ZZDDShipActor.h"
AZZDDShipActor::AZZDDShipActor()
{
    SetActorScale3D(FVector(2.0f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZDD"));
}
