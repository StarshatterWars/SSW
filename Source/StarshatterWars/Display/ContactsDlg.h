#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "ContactsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UContactsDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
public:
    void SetMissionHost(class UMissionBriefingDlg* Host);
protected:
    TWeakObjectPtr<class UMissionBriefingDlg> MissionHost;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
};
