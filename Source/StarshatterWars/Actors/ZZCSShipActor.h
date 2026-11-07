#pragma once
#include "CapitalShipActor.h"
#include "ZZCSShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZCSShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AZZCSShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
