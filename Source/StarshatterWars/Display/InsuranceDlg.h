#pragma once
#include "BaseScreen.h"
#include "InsuranceDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UInsuranceDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    void SetHull(FGuid Id) { Hull = Id; }
protected:
    TSharedRef<SWidget> RebuildWidget() override;
private:
    FGuid Hull;
};
