#pragma once
#include "CapitalShipActor.h"
#include "TankerBShipActor.generated.h"

/** Presentation class for TankerB.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATankerBShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATankerBShipActor();
};
