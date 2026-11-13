#pragma once
#include "ShipActor.h"
#include "StationActor.h"
#include "S6ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AS6ShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AS6ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
