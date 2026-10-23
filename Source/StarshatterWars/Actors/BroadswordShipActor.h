#pragma once
#include "CapitalShipActor.h"
#include "BroadswordShipActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API ABroadswordShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ABroadswordShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
