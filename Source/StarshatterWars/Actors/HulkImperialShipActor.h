#pragma once
#include "ShipActor.h"
#include "HulkImperialShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AHulkImperialShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AHulkImperialShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
