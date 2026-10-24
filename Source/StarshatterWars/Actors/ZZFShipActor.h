#pragma once
#include "FighterActor.h"
#include "ZZFShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZFShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AZZFShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
