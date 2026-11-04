#pragma once
#include "IM4300ShipActor.h"
#include "CargoBShipActor.generated.h"

/** Presentation class for CargoB.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACargoBShipActor : public AIM4300ShipActor
{
    GENERATED_BODY()
public:
    ACargoBShipActor();
};
