#pragma once
#include "CapitalShipActor.h"
#include "TransportShipActor.generated.h"

/** Presentation class for Transport.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATransportShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATransportShipActor();
};
