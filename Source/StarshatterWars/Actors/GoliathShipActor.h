#pragma once

#include "CoreMinimal.h"
#include "CarrierActor.h"
#include "GoliathShipActor.generated.h"

// Goliath.def class: Carrier. Flight-deck simulation remains owned by legacy Ship.
// Two flight decks are specified in Goliath.def: one launch and one recovery.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AGoliathShipActor : public ACarrierActor
{
    GENERATED_BODY()
public:
    AGoliathShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyGoliathDefaults();
    void ApplyGoliathFixedPoints();
};
