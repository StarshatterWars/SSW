#pragma once
#include "Commandlets/Commandlet.h"
#include "SSWPlayerWalletCommandlet.generated.h"
// Explicit offline maintenance of the selected local pilot, with backups.
UCLASS()
class USSWPlayerWalletCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    USSWPlayerWalletCommandlet();
    int32 Main(const FString& Params) override;
};
