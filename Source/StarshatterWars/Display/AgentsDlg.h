#pragma once

#include "BaseScreen.h"
#include "AgentsDlg.generated.h"

class SBox;
class SVerticalBox;
class UMissionsDlg;

UCLASS(Blueprintable)
class STARSHATTERWARS_API UAgentsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    void UseStandaloneFrame() { bStandaloneFrame = true; CanLaunch.BindLambda([] { return true; }); }
    void SetCanLaunch(TDelegate<bool()> InCanLaunch) { CanLaunch = MoveTemp(InCanLaunch); }
    void RefreshBoard();

protected:
    TSharedRef<SWidget> RebuildWidget() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    void RequestMission(bool bCampaign = false);
    void StartMission(int32 MissionId);
    void OpenDetails(int32 MissionId, bool bOffer);
    void CloseDetails();

    bool bStandaloneFrame = false;
    TDelegate<bool()> CanLaunch;
    FText Feedback;
    FName SelectedAgent;
    TArray<TSharedPtr<FName>> AgentChoices;
    TSharedPtr<SVerticalBox> Board;
    TSharedPtr<SBox> DetailLayer;
    UPROPERTY(Transient) TObjectPtr<UMissionsDlg> Details;
};
