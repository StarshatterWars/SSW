#pragma once
#include "CapitalShipActor.h"
#include "StationActor.h"
#include "HarmonyShipActor.generated.h"

/** Presentation class for Harmony.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AHarmonyShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AHarmonyShipActor();
};
