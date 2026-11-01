#pragma once
#include "CoreMinimal.h"
#include "ShipActor.h"
#include "DroneActor.generated.h"

/** Uncrewed ship presentation; legacy Ship/AI remain authoritative. */
UCLASS(Blueprintable)
class STARSHATTERWARS_API ADroneActor : public AShipActor
{
    GENERATED_BODY()
public:
    ADroneActor();
    virtual void OnConstruction(const FTransform& Transform) override;
private:
    void ApplyDroneDefaults();
};
