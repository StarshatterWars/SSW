#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "OrionShipActor.generated.h"

// Structural/model defaults from Orion.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AOrionShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AOrionShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyOrionDefaults();
    void ApplyOrionFixedPoints();
};
