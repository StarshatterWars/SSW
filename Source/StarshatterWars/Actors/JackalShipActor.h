#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "JackalShipActor.generated.h"

// Structural/model defaults from Jackal.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AJackalShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AJackalShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyJackalDefaults();
    void ApplyJackalFixedPoints();
};
