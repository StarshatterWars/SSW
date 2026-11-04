#pragma once
#include "CapitalShipActor.h"
#include "TelmeraStarportShipActor.generated.h"

/** Presentation class for TelmeraStarport.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATelmeraStarportShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATelmeraStarportShipActor();
};
