#pragma once

#include "CoreMinimal.h"
#include "ShipActor.h"
#include "ViperShipActor.generated.h"

// Structural/model defaults from Viper.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AViperShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AViperShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyViperDefaults();
    void ApplyViperFixedPoints();
};
