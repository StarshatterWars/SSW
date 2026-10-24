#pragma once
#include "FighterActor.h"
#include "ZZAShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZZAShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AZZAShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
