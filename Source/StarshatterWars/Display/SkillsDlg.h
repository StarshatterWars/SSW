#pragma once
#include "BaseScreen.h"
#include "SkillsDlg.generated.h"
UCLASS(Blueprintable)
class STARSHATTERWARS_API USkillsDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
