#include "ZZFShipActor.h"
AZZFShipActor::AZZFShipActor()
{
    SetActorScale3D(FVector(0.25f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("ZZF"));
}
