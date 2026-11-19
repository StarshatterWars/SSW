#pragma once
#include "BaseScreen.h"
#include "StationServiceCatalog.h"
#include "StationServicesDlg.generated.h"

DECLARE_DELEGATE_OneParam(FOpenStationService,ESSWStationService);
UCLASS(Blueprintable)
class STARSHATTERWARS_API UStationServicesDlg : public UBaseScreen
{
    GENERATED_BODY()
public:
    FOpenStationService OnOpenService;
    void SetStation(FName Station) { ViewedStation=Station; }
    FName GetStation() const { return ViewedStation; }
private:
    UPROPERTY(Transient) FName ViewedStation;
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
