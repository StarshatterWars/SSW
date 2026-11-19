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
    FSimpleDelegate OnOpenAwards;
    FSimpleDelegate OnOpenWallet;
    FSimpleDelegate OnOpenGuilds;
    TSharedRef<SWidget> BuildPanelContent(UGameInstance* Instance) const;

protected:
    TSharedRef<SWidget> RebuildWidget() override;
    void NativeConstruct() override;
    void NativeDestruct() override;
    void NativeTick(const FGeometry&,float) override;
private:
    FDelegateHandle JournalChangedHandle;
    bool bJournalDirty=false;
    TSharedPtr<class SBox> ContentHost;
};
