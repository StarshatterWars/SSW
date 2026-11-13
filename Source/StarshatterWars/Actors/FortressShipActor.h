#pragma once
#include "CapitalShipActor.h"
#include "StationActor.h"
#include "FortressShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AFortressShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AFortressShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
