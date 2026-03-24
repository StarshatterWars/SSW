/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdMsgDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdMsgDlg (Unreal port of CmdMsgDlg)
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CmdMsgDlg.generated.h"

// Forward declarations (UMG)
class UButton;
class UTextBlock;
class URichTextBlock;

// Forward declaration (your campaign screen)
class UCmpnScreen;

UCLASS()
class STARSHATTERWARS_API UCmdMsgDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    UCmdMsgDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    void SetManager(UCmpnScreen* InManager);

    // Show/hide
    void ShowMsgDlg();
    void HideMsgDlg();

    // Legacy-like API (keeps call sites simple)
    void SetTitleText(const FString& InTitle);
    void SetMessageText(const FString& InMessage);

protected:
    // Button handler
    UFUNCTION() void OnCloseClicked();

private:
    void UpdateFocusIfVisible();
    void HandleKeyboardShortcuts();

private:
    UCmpnScreen* Manager = nullptr;

    // UMG bindings
    UPROPERTY(meta = (BindWidgetOptional)) 
    UTextBlock* txt_title = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) 
    URichTextBlock* txt_message_rich = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) 
    UTextBlock* txt_message = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_close = nullptr;

    bool bExitLatch = false;
    bool bWantsFocus = false;
};
