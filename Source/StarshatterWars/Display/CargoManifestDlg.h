#pragma once

#include "BaseScreen.h"
#include "CargoManifestDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UCargoManifestDlg : public UBaseScreen
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
