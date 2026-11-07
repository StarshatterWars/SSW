#pragma once
#include "MasterBuildingActor.h"
#include "Gnd_aaa2ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AGnd_aaa2ShipActor : public AMasterBuildingActor
{
    GENERATED_BODY()
public:
    AGnd_aaa2ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
