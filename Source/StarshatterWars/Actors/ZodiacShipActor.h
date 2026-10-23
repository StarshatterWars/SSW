#pragma once
#include "FighterActor.h"
#include "ZodiacShipActor.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API AZodiacShipActor : public AFighterActor
{
    GENERATED_BODY()
public:
    AZodiacShipActor();
protected:
    virtual void ApplyAuthoredAttachmentDefaults() override;
};
