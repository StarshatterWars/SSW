#pragma once
#include "BaseScreen.h"
#include "TransactionDlg.generated.h"

class UStarshatterPlayerSubsystem;

// Personal wallet. The simulation owns balances; this screen only queries them.
UCLASS(Blueprintable)
class STARSHATTERWARS_API UTransactionDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    FSimpleDelegate OnOpenMarketplace;
    FSimpleDelegate OnOpenContracts;
    FSimpleDelegate OnOpenProduction;
    FSimpleDelegate OnOpenHangar;
    static TSharedRef<SWidget> BuildWalletContent(UStarshatterPlayerSubsystem* Players, const UTransactionDlg* Owner=nullptr);
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
