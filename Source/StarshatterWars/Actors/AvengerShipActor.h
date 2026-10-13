#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "AvengerShipActor.generated.h"

// Structural/model defaults from Avenger.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAvengerShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AAvengerShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyAvengerDefaults();
    void ApplyAvengerFixedPoints();
};
