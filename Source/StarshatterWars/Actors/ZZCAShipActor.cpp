#include "ZZCAShipActor.h"
AZZCAShipActor::AZZCAShipActor()
{
    SetActorScale3D(FVector(4.0f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZCA"));
}
