#pragma once
#include "CapitalShipActor.h"
#include "TankerAShipActor.generated.h"

/** Presentation class for TankerA.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATankerAShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATankerAShipActor();
};
