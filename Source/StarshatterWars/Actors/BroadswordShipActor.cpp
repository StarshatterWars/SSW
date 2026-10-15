#include "BroadswordShipActor.h"
ABroadswordShipActor::ABroadswordShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Broadsword"));
    SetActorScale3D(FVector(3.5f));
    ConfigureTurretsFromShipDef(TEXT("Broadsword"));
}
