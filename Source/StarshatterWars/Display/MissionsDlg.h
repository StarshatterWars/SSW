#pragma once

#include "BaseScreen.h"
#include "MissionsDlg.generated.h"

UCLASS()
class STARSHATTERWARS_API UMissionsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    void SetDetails(FText InTitle, FText InBrief, FSimpleDelegate InAccept, FSimpleDelegate InDecline, FSimpleDelegate InClose);

protected:
    TSharedRef<SWidget> RebuildWidget() override;

private:
    FText Title;
    FText Brief;
    FSimpleDelegate Accept;
    FSimpleDelegate Decline;
    FSimpleDelegate Close;
};
