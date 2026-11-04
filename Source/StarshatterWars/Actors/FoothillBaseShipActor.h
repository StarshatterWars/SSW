#pragma once
#include "CapitalShipActor.h"
#include "FoothillBaseShipActor.generated.h"

/** Presentation class for FoothillBase.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFoothillBaseShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AFoothillBaseShipActor();
};
