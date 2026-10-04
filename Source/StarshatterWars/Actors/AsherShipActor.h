#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "AsherShipActor.generated.h"

// Structural/model defaults from Asher.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAsherShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AAsherShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyAsherDefaults();
    void ApplyAsherFixedPoints();
};
