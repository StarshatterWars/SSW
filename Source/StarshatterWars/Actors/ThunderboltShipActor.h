#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "ThunderboltShipActor.generated.h"

// Structural/model defaults from Thunderbolt.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AThunderboltShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AThunderboltShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyThunderboltDefaults();
    void ApplyThunderboltFixedPoints();
};
