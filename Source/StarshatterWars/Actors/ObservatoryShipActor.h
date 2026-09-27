#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "ObservatoryShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AObservatoryShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AObservatoryShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyObservatoryDefaults();
    void ApplyObservatoryFixedPoints();
};
