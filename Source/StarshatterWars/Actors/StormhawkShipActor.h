#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "StormhawkShipActor.generated.h"

// Structural/model defaults from Stormhawk.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AStormhawkShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AStormhawkShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyStormhawkDefaults();
    void ApplyStormhawkFixedPoints();
};
