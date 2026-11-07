#pragma once
#include "CapitalShipActor.h"
#include "ZZCAShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZCAShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AZZCAShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
