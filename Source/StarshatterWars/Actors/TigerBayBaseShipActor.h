#pragma once
#include "CapitalShipActor.h"
#include "TigerBayBaseShipActor.generated.h"

/** Presentation class for TigerBayBase.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATigerBayBaseShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATigerBayBaseShipActor();
};
