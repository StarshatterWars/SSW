#pragma once

#include "BaseScreen.h"
#include "ContractsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UContractsDlg : public UBaseScreen
{
    GENERATED_BODY()

protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
