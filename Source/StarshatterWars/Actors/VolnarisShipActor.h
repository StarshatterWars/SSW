#pragma once
#include "CapitalShipActor.h"
#include "VolnarisShipActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API AVolnarisShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    AVolnarisShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
