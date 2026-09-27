#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdOrdersDlg.generated.h"

class UTextBlock;
class UCmdDlg;

UCLASS()
class STARSHATTERWARS_API UCmdOrdersDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    UCmdOrdersDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

public:
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowOrdersDlg();
    void SetCampaignOrders();

protected:
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* CampaignNameText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* DescriptionText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* SituationText = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders1Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders2Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders3Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* Orders4Text = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* LocationSystemText = nullptr;

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;
};