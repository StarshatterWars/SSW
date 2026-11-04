#pragma once
#include "CameraPodActor.h"
#include "CameraPodShipActor.generated.h"

/** Presentation class for CameraPod.def. Simulation remains data-driven. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACameraPodShipActor : public ACameraPodActor
{
    GENERATED_BODY()
public:
    ACameraPodShipActor();
};
