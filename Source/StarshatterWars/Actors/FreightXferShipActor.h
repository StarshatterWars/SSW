#pragma once
#include "CapitalShipActor.h"
#include "FreightXferShipActor.generated.h"

/** Presentation class for FreightXfer.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFreightXferShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AFreightXferShipActor();
};
