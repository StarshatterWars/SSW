#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "GoliathShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AGoliathShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AGoliathShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyGoliathDefaults();
    void ApplyGoliathFixedPoints();
};
