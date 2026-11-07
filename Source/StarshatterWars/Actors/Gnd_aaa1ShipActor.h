#pragma once
#include "MasterBuildingActor.h"
#include "Gnd_aaa1ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AGnd_aaa1ShipActor : public AMasterBuildingActor
{
    GENERATED_BODY()
public:
    AGnd_aaa1ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
