#pragma once
#include "ShipActor.h"
#include "S5ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AS5ShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AS5ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
