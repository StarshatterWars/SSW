#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "IntruderShipActor.generated.h"

// Structural/model defaults from Intruder.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AIntruderShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AIntruderShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyIntruderDefaults();
    void ApplyIntruderFixedPoints();
};
