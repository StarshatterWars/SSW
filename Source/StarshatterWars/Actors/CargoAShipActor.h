#pragma once
#include "IM3500ShipActor.h"
#include "CargoAShipActor.generated.h"

/** Presentation class for CargoA.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACargoAShipActor : public AIM3500ShipActor
{
    GENERATED_BODY()
public:
    ACargoAShipActor();
};
