#pragma once
#include "ShipActor.h"
#include "CutlassShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACutlassShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    ACutlassShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
