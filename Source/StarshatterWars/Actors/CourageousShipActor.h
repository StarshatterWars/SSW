#pragma once

#include "CoreMinimal.h"
#include "CapitalShipActor.h"
#include "CourageousShipActor.generated.h"

// Structural/model defaults from Courageous.def. Simulation remains data-driven.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACourageousShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ACourageousShipActor();
    virtual void OnConstruction(const FTransform& Transform) override;
protected:
    void ApplyCourageousDefaults();
    void ApplyCourageousFixedPoints();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
