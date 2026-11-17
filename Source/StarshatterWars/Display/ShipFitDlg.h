#pragma once
#include "BaseScreen.h"
#include "ShipFitDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API UShipFitDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetHull(FGuid Id) { Hull = Id; }
protected:
    TSharedRef<SWidget> RebuildWidget() override;
private:
    FGuid Hull;
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> Preview;
};
