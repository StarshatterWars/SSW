#pragma once
#include "FighterActor.h"
#include "SaberShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API ASaberShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    ASaberShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
