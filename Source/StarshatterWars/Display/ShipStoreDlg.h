#pragma once

#include "BaseScreen.h"
#include "ShipStoreDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UShipStoreDlg : public UBaseScreen
{
    GENERATED_BODY()

protected:
    TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UShipBriefingPreview> Preview;
};

