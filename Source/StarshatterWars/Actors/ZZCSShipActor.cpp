#include "ZZCSShipActor.h"
AZZCSShipActor::AZZCSShipActor()
{
    SetActorScale3D(FVector(7.5f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZCS"));
}
