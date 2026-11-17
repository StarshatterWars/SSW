#pragma once

#include "BaseScreen.h"
#include "FlightLogbookDlg.generated.h"

UCLASS()
class STARSHATTERWARS_API UFlightLogbookDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    FSimpleDelegate OnOpenHangar;
    FSimpleDelegate OnOpenSkills;
    TSharedRef<SWidget> BuildPanelContent(UGameInstance* Instance) const;

protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
