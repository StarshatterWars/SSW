#pragma once
#include "DroneActor.h"
#include "TargetDroneShipActor.generated.h"

/** Presentation class for TargetDrone.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATargetDroneShipActor : public ADroneActor
{
    GENERATED_BODY()
public:
    ATargetDroneShipActor();
};
