#pragma once
#include "CapitalShipActor.h"
#include "FortressShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFortressShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AFortressShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
