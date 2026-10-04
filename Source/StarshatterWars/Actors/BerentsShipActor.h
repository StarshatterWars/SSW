#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "BerentsShipActor.generated.h"

// Structural/model defaults from Berents.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ABerentsShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ABerentsShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyBerentsDefaults();
    void ApplyBerentsFixedPoints();
};
