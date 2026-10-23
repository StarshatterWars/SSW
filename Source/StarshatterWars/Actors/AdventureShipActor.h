#pragma once
#include "ShipActor.h"
#include "AdventureShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAdventureShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AAdventureShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
