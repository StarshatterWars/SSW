#pragma once

#include "Commandlets/Commandlet.h"
#include "SSWDesignIngestionCommandlet.generated.h"

UCLASS()
class STARSHATTERWARS_API USSWDesignIngestionCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    USSWDesignIngestionCommandlet();
    int32 Main(const FString& Params) override;

private:
#if WITH_EDITOR
    bool ImportSingleShipDefinition(const FString& Definition);
#endif
};
