#include "SpectreShipActor.h"
ASpectreShipActor::ASpectreShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Spectre"));
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("Spectre"));
}
