#pragma once

#include "CapitalShipActor.h"
#include "SpectreShipActor.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API ASpectreShipActor : public ACapitalShipActor
{
    GENERATED_BODY()
public:
    ASpectreShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
