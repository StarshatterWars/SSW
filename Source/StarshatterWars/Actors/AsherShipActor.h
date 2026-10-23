#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "AsherShipActor.generated.h"

// Structural/model defaults from Asher.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAsherShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AAsherShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyAsherDefaults();
    void ApplyAsherFixedPoints();
};
