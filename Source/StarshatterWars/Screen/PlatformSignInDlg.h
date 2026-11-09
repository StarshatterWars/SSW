#pragma once

#include "Blueprint/UserWidget.h"
#include "PlatformSignInDlg.generated.h"

UCLASS()
class STARSHATTERWARS_API UPlatformSignInDlg : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
