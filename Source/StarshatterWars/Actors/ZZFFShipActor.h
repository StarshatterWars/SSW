#pragma once
#include "ShipActor.h"
#include "ZZFFShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZFFShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AZZFFShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
