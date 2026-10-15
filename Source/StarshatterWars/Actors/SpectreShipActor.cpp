#include "SpectreShipActor.h"
ASpectreShipActor::ASpectreShipActor()
{
    SetActorScale3D(FVector(2.5f));
    ConfigureTurretsFromShipDef(TEXT("Spectre"));
}
