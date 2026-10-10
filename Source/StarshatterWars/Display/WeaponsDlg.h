#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "WeaponsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UWeaponsDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
};
