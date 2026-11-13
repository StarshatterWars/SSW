#pragma once
#include "CapitalShipActor.h"
#include "StationActor.h"
#include "JanekStationShipActor.generated.h"

/** Presentation class for JanekStation.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AJanekStationShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AJanekStationShipActor();
};
