#pragma once

#include "BaseScreen.h"
#include "ProductionDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UProductionDlg : public UBaseScreen
{
    GENERATED_BODY()

protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
