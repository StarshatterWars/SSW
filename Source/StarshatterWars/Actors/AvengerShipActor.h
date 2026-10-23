#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "AvengerShipActor.generated.h"

// Structural/model defaults from Avenger.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAvengerShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AAvengerShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyAvengerDefaults();
    void ApplyAvengerFixedPoints();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
