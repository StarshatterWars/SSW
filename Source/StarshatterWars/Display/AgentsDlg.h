#pragma once

#include "BaseScreen.h"
#include "AgentDirectoryRules.h"
#include "AgentsDlg.generated.h"

class SBox;
class SVerticalBox;
class UMissionsDlg;
template<typename ItemType> class SComboBox;

UCLASS(Blueprintable)
class STARSHATTERWARS_API UAgentsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    void UseStandaloneFrame() { bStandaloneFrame = true; CanLaunch.BindLambda([] { return true; }); }
    void SetCanLaunch(TDelegate<bool()> InCanLaunch) { CanLaunch = MoveTemp(InCanLaunch); }
    void RefreshBoard();

protected:
    void NativeConstruct() override;
    void NativeDestruct() override;
    void NativeTick(const FGeometry&,float) override;
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    void RequestMission(bool bCampaign = false);
    void StartMission(int32 MissionId);
    bool CanStartMission(int32 MissionId) const;
    void OpenIntroduction(FName Id);
    void OpenDetails(int32 MissionId, bool bOffer);
    void CloseDetails();
    void RefreshDirectory();
    void SelectDirectoryAgent();

    FDelegateHandle JournalChangedHandle;
    bool bJournalDirty=false;
    bool bStandaloneFrame = false;
    TDelegate<bool()> CanLaunch;
    FText Feedback;
    FName SelectedAgent;
    FName SelectedSystem;
    EAgentDirectoryScope DirectoryScope = EAgentDirectoryScope::SameSystem;
    bool bRefreshingDirectory = false;
    TArray<TSharedPtr<FName>> SystemChoices;
    TArray<TSharedPtr<FName>> AgentChoices;
    TSharedPtr<SComboBox<TSharedPtr<FName>>> SystemPicker;
    TSharedPtr<SComboBox<TSharedPtr<FName>>> AgentPicker;
    TSharedPtr<SVerticalBox> Board;
    TSharedPtr<SBox> DetailLayer;
    UPROPERTY(Transient) TObjectPtr<UMissionsDlg> Details;
};
