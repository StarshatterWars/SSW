#pragma once
#include "BaseScreen.h"
#include "AwardsDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API UAwardsDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
