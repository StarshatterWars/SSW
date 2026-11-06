#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdOrdersDlg.generated.h"

class UTextBlock;
class UCmdDlg;
class UImage;
class USizeBox;

/** Operations > Orders. Layout is constructed in C++ and does not require Blueprint bindings. */
UCLASS()
class STARSHATTERWARS_API UCmdOrdersDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    UCmdOrdersDlg(const FObjectInitializer& ObjectInitializer);
    virtual void NativeConstruct() override;

    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowOrdersDlg();
    void SetCampaignOrders();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    // Retained for compatibility with existing WBP_CmdOrdersDlg bindings.
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* CampaignNameText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* DescriptionText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* SituationText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders1Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders2Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders3Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders4Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* LocationSystemText = nullptr;

    UPROPERTY() UCmdDlg* ParentCmdDlg = nullptr;

private:
    UPROPERTY(Transient) TObjectPtr<USizeBox> CampaignArtFrame = nullptr;
    UPROPERTY(Transient) TObjectPtr<UImage> CampaignArt = nullptr;
};
