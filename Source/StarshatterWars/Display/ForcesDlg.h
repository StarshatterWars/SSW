#pragma once
#include "BaseScreen.h"
#include "ForcesDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API UForcesDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
