#pragma once
#include "ShipActor.h"
#include "Mine1ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AMine1ShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AMine1ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
