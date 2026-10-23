#pragma once

#include "CoreMinimal.h"
#include "FighterActor.h"
#include "EagleShipActor.generated.h"

// Structural/model defaults from Eagle.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AEagleShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AEagleShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyEagleDefaults();
    void ApplyEagleFixedPoints();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
