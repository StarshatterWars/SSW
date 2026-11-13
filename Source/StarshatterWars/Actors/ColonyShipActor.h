#pragma once
#include "CapitalShipActor.h"
#include "StationActor.h"
#include "ColonyShipActor.generated.h"

/** Presentation class for Colony.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AColonyShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AColonyShipActor();
};
