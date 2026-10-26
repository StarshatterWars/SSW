#pragma once
#include "CoreMinimal.h"
#include "SingleMissionDlg.h"
#include "CampaignMissionDlg.generated.h"

// Campaign tab presentation of the current Operations mission roster.
// Shares the Single/Custom UI shell; never switches the selected campaign.
UCLASS()
class STARSHATTERWARS_API UCampaignMissionDlg : public USingleMissionDlg
{
    GENERATED_BODY()
public:
    UCampaignMissionDlg(const FObjectInitializer& ObjectInitializer);
    virtual bool IsCampaignMissionView() const override { return true; }
};
