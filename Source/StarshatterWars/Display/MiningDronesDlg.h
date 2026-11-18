#pragma once
#include "BaseScreen.h"
#include "MiningDronesDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API UMiningDronesDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
