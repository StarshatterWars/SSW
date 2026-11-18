#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "ObjectivesDlg.generated.h"

// WBP_Objectives: PanelTitle TextBlock and RuntimeHost SizeBox in a CanvasPanel.
UCLASS(Blueprintable)
class STARSHATTERWARS_API UObjectivesDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
public:
    void SetMissionContext(class UMissionBriefingDlg* InBriefing, bool bShowPackage = false);
protected:
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
    virtual bool UsesFlightPanelStyle() const override { return true; }
    virtual bool ShowsFooter() const override { return false; }
    virtual void NativeDestruct() override;
    bool bIncludePackage = false;
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> Rehearsal;
    UPROPERTY(Transient) TObjectPtr<class UMissionPackageDlg> PackagePanel;
    UPROPERTY(Transient) TWeakObjectPtr<class UMissionBriefingDlg> Briefing;
};
