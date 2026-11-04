#pragma once
#include "CapitalShipActor.h"
#include "TanisBaseShipActor.generated.h"

/** Presentation class for TanisBase.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATanisBaseShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATanisBaseShipActor();
};
