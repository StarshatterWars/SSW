#pragma once
#include "BaseScreen.h"
#include "BlueprintsDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API UBlueprintsDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
