#pragma once
#include "CapitalShipActor.h"
#include "RadixBaseShipActor.generated.h"

/** Presentation class for RadixBase.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ARadixBaseShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ARadixBaseShipActor();
};
