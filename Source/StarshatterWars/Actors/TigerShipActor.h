#pragma once
#include "CapitalShipActor.h"
#include "TigerShipActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API ATigerShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ATigerShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
