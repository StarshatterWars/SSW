/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI
    FILE:         CmdMsgDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Campaign message dialog (modal).
    Displays simple text messages and blocks input until dismissed.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdMsgDlg.generated.h"

class UTextBlock;
class URichTextBlock;
class UCmpnScreen;

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

    void SetCmpnScreen(UCmpnScreen* InScreen);

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

    UPROPERTY()
    TObjectPtr<UCmpnScreen> CmpnScreen = nullptr;

    bool bExitLatch = false;
    bool bWantsFocus = false;
};