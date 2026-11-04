#pragma once
#include "ShipActor.h"
#include "Relay1ShipActor.generated.h"

/** Presentation class for Relay1.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ARelay1ShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ARelay1ShipActor();
};
