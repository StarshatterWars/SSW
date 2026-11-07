#pragma once
#include "CapitalShipActor.h"
#include "ZZDDShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZDDShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AZZDDShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
