#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "RazorShipActor.generated.h"

// Structural/model defaults from Razor.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ARazorShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    ARazorShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyRazorDefaults();
    void ApplyRazorFixedPoints();
};
