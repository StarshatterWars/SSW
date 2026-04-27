#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "FarcasterShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AFarcasterShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AFarcasterShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyFarcasterDefaults();
    void ApplyFarcasterFixedPoints();
};