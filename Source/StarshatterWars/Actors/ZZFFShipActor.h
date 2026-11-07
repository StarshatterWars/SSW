#pragma once
#include "CapitalShipActor.h"
#include "ZZFFShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZFFShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AZZFFShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
