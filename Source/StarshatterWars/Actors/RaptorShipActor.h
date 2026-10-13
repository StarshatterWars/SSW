#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "RaptorShipActor.generated.h"

// Structural/model defaults from Raptor.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ARaptorShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ARaptorShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyRaptorDefaults();
    void ApplyRaptorFixedPoints();
};
