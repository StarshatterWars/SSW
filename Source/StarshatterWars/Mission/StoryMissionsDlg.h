#pragma once

#include "CampaignMissionDlg.h"
#include "StoryMissionsDlg.generated.h"

UCLASS()
class STARSHATTERWARS_API UStoryMissionsDlg : public UCampaignMissionDlg
{
    GENERATED_BODY()

public:
    explicit UStoryMissionsDlg(const FObjectInitializer& ObjectInitializer);
    virtual bool IsCampaignMissionView() const override { return false; }
    virtual bool IsStoryMissionView() const override { return true; }
};
