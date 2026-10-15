#include "ZodiacShipActor.h"
AZodiacShipActor::AZodiacShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Zodiac"));
    SetActorScale3D(FVector(0.45f));
    ConfigureTurretsFromShipDef(TEXT("Zodiac"));
}
