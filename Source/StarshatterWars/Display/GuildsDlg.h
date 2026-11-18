#pragma once
#include "BaseScreen.h"
#include "GuildsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API UGuildsDlg : public UBaseScreen
{
    GENERATED_BODY()
protected:
    TSharedRef<SWidget> RebuildWidget() override;
};
