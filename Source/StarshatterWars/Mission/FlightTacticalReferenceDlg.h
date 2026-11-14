#pragma once

#include "Blueprint/UserWidget.h"
#include "FlightTacticalReferenceDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UFlightTacticalReferenceDlg : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
