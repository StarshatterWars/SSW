#include "HarmonyShipActor.h"

AHarmonyShipActor::AHarmonyShipActor()
{
    SetActorScale3D(FVector(10.0f));
    // Bound ships build their lights, engines and mounts from the parsed design.
    NavLightDefs.Reset();
}
