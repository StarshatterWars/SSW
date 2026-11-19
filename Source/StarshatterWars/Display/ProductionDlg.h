#pragma once

#include "BaseScreen.h"
#include "ProductionDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UProductionDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    void ShowBlueprints();
private:
    TWeakPtr<class SWidgetSwitcher> ProductionPages;
    UPROPERTY(Transient) TObjectPtr<class UBlueprintsDlg> BlueprintBrowser;
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
