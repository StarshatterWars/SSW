#pragma once

#include "Blueprint/UserWidget.h"
#include "LiveOptionsDlg.generated.h"

UCLASS(Blueprintable)
class STARSHATTERWARS_API ULiveOptionsDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    void SaveSettings();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    UPROPERTY(Transient) TObjectPtr<class UVideoDlg> FullVideo;
    UPROPERTY(Transient) TObjectPtr<class UGameOptionsDlg> FullGameplay;
    UPROPERTY(Transient) TObjectPtr<class UJoyDlg> Joystick;
};
