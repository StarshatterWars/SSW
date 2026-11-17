#pragma once
#include "BaseScreen.h"
#include "HangarDlg.generated.h"

DECLARE_DELEGATE_OneParam(FOpenShipInsurance, FGuid);

UCLASS(Blueprintable)
class STARSHATTERWARS_API UHangarDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    FOpenShipInsurance OnInsurance;
    FOpenShipInsurance OnLoadout;
    FOpenShipInsurance OnFit;
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
