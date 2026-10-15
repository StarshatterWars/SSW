#include "SaberShipActor.h"
ASaberShipActor::ASaberShipActor()
{
    SetActorScale3D(FVector(0.2f));
    ConfigureWeaponHardpointsFromShipDef(TEXT("Saber"));
}
