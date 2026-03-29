#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdMsgDlg.generated.h"

class UTextBlock;
class URichTextBlock;

UCLASS()
class STARSHATTERWARS_API UCmdMsgDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    UCmdMsgDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
    void ShowMsgDlg();
    void HideMsgDlg();

    void SetTitleText(const FString& InTitle);
    void SetMessageText(const FString& InMessage);

private:
    void UpdateFocusIfVisible();
    void HandleKeyboardShortcuts();

private:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* TitleText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    URichTextBlock* MessageTextBlock = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MessageText = nullptr;

    bool bExitLatch = false;
    bool bWantsFocus = false;
};