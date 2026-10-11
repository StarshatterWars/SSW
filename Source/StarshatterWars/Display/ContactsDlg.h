#pragma once
#include "CoreMinimal.h"
#include "InMissionPanelBase.h"
#include "ContactsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UContactsDlg : public UInMissionPanelBase
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> CreatePanelContent() override;
    virtual FText GetPanelCaption() const override;
};
