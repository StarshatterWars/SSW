#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "CobraShipActor.generated.h"

// Structural/model defaults from Cobra.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACobraShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    ACobraShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyCobraDefaults();
    void ApplyCobraFixedPoints();
};
