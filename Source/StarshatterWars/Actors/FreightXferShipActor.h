#pragma once
#include "CapitalShipActor.h"
#include "StationActor.h"
#include "FreightXferShipActor.generated.h"

/** Presentation class for FreightXfer.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFreightXferShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AFreightXferShipActor();
};
