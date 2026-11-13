#pragma once
#include "ShipActor.h"
#include "StationActor.h"
#include "S7ShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AS7ShipActor : public AStationActor
{
    GENERATED_BODY()
public:
    AS7ShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
