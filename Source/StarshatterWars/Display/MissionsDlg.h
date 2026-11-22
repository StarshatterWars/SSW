#pragma once

#include "BaseScreen.h"
#include "MissionsDlg.generated.h"

UCLASS()
class STARSHATTERWARS_API UMissionsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    void SetDetails(FText InTitle, FText InBrief, FSimpleDelegate InAccept, FSimpleDelegate InDecline, FSimpleDelegate InClose);

    void SetAssignmentMetadata(FText InAgent,FText InStatus,FText InExpiry) { MetadataAgent=MoveTemp(InAgent);MetadataStatus=MoveTemp(InStatus);MetadataExpiry=MoveTemp(InExpiry); }
    void SetStartMission(FSimpleDelegate Action, TDelegate<bool()> CanStart) { StartMission=MoveTemp(Action); CanStartMission=MoveTemp(CanStart); }

    void SetLaunchFeedback(FText Message) { LaunchFeedback = MoveTemp(Message); }

    void SetSupplement(TSharedRef<SWidget> Content) { Supplement=Content; }

protected:
    TSharedRef<SWidget> RebuildWidget() override;
    void NativeDestruct() override;
    void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    TSharedPtr<SWidget> Supplement;
    FText MetadataAgent,MetadataStatus,MetadataExpiry;
    FText LaunchFeedback;
    FText Title;
    FText Brief;
    FSimpleDelegate Accept;
    FSimpleDelegate Decline;
    FSimpleDelegate Close;
    FSimpleDelegate StartMission;
    TDelegate<bool()> CanStartMission;
};
