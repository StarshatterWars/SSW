#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "DevastatorShipActor.generated.h"

// Structural/model defaults from Devastator.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ADevastatorShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ADevastatorShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyDevastatorDefaults();
    void ApplyDevastatorFixedPoints();
};
