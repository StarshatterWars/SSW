#pragma once

#include "BaseScreen.h"
#include "MarketplaceDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UMarketplaceDlg : public UBaseScreen
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
