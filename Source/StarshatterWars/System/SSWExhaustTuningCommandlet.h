#pragma once
#include "Commandlets/Commandlet.h"
#include "SSWExhaustTuningCommandlet.generated.h"
/** Reproducible editor asset recipe: -run=SSWExhaustTuning. */
UCLASS()
class USSWExhaustTuningCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    USSWExhaustTuningCommandlet();
    virtual int32 Main(const FString& Params) override;
};
