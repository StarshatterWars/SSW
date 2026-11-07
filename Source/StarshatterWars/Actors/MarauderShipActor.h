#pragma once
#include "CapitalShipActor.h"
#include "MarauderShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AMarauderShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AMarauderShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
