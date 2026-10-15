#include "ZodiacShipActor.h"
AZodiacShipActor::AZodiacShipActor()
{
    SetActorScale3D(FVector(0.45f));
    ConfigureTurretsFromShipDef(TEXT("Zodiac"));
}
