#pragma once
#include "MasterBuildingActor.h"
#include "Gnd_sam1ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AGnd_sam1ShipActor : public AMasterBuildingActor
{
    GENERATED_BODY()
public:
    AGnd_sam1ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
