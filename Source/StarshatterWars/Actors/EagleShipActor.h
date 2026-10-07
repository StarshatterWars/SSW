#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "EagleShipActor.generated.h"

// Structural/model defaults from Eagle.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AEagleShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AEagleShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyEagleDefaults();
    void ApplyEagleFixedPoints();
};
