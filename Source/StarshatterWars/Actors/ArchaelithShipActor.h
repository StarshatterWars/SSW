#pragma once
#include "ShipActor.h"
#include "ArchaelithShipActor.generated.h"

/** Presentation class for Archaelith.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AArchaelithShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AArchaelithShipActor();
};
