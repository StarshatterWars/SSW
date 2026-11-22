#pragma once
#include "BaseScreen.h"
#include "ShipSkinDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UShipSkinDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> Preview;
};
