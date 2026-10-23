#pragma once
#include "ShipActor.h"
#include "FortressShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFortressShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AFortressShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
