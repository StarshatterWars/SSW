#pragma once
#include "ShipActor.h"
#include "ZZAShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZAShipActor : public AShipActor
{
    GENERATED_BODY()
public:
    AZZAShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
