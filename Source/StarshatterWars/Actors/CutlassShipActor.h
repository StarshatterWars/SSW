#pragma once
#include "FighterActor.h"
#include "CutlassShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API ACutlassShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    ACutlassShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
