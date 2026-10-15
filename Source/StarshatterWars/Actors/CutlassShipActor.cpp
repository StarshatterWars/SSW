#include "CutlassShipActor.h"
ACutlassShipActor::ACutlassShipActor()
{
    SetActorScale3D(FVector(0.24f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("Cutlass"));
}
