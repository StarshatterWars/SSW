#include "ZZFFShipActor.h"
AZZFFShipActor::AZZFFShipActor()
{
    SetActorScale3D(FVector(8.0f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZFF"));
}
