#pragma once
#include "CapitalShipActor.h"
#include "HarmonyShipActor.generated.h"

/** Presentation class for Harmony.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AHarmonyShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AHarmonyShipActor();
};
