#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "FalconShipActor.generated.h"

// Structural/model defaults from Falcon.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFalconShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AFalconShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyFalconDefaults();
    void ApplyFalconFixedPoints();
};
