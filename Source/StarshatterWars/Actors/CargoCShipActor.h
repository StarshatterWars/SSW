#pragma once
#include "IM2800ShipActor.h"
#include "CargoCShipActor.generated.h"

/** Presentation class for CargoC.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACargoCShipActor : public AIM2800ShipActor
{
    GENERATED_BODY()
public:
    ACargoCShipActor();
};
