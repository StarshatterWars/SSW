#pragma once
#include "ShipActor.h"
#include "HulkVendettaShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AHulkVendettaShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AHulkVendettaShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
