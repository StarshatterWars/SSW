#pragma once
#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "WolfShipActor.generated.h"

UCLASS()
class STARSHATTERWARS_API AWolfShipActor : public AShipActor
{
    GENERATED_BODY()

public:
    AWolfShipActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    void ApplyWolfDefaults();
    void ApplyWolfFixedPoints();
};