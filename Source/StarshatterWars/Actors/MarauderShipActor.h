#pragma once
#include "ShipActor.h"
#include "MarauderShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AMarauderShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AMarauderShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
