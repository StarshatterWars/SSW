#pragma once
#include "CarrierActor.h"
#include "AdventureShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AAdventureShipActor : public ACarrierActor
{
    GENERATED_BODY()
public:
    AAdventureShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
