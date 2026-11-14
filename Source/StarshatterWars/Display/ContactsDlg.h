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
    virtual void NativeConstruct() override;
    TWeakObjectPtr<class UMissionBriefingDlg> MissionHost;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
    virtual bool UsesFlightPanelStyle() const override { return true; }
    virtual bool ShowsFooter() const override { return false; }
private:
    TWeakPtr<class SWidget> ContactList;
};
