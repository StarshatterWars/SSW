#pragma once
#include "Commandlets/Commandlet.h"
#include "ResetCampaignAcceptanceCommandlet.generated.h"

UCLASS()
class UResetCampaignAcceptanceCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UResetCampaignAcceptanceCommandlet();
    virtual int32 Main(const FString& Params) override;
};
