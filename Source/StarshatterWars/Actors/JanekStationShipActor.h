#pragma once
#include "CapitalShipActor.h"
#include "JanekStationShipActor.generated.h"

/** Presentation class for JanekStation.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AJanekStationShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AJanekStationShipActor();
};
