/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdMsgDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdMsgDlg implementation (Unreal port)
*/

#include "CmdMsgDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

// Your screen manager
#include "CmpnScreen.h"

UCmdMsgDlg::UCmdMsgDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdMsgDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (btn_close)
        btn_close->OnClicked.AddDynamic(this, &UCmdMsgDlg::OnCloseClicked);

    bExitLatch = false;
    bWantsFocus = true;
}

void UCmdMsgDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() == ESlateVisibility::Visible)
    {
        UpdateFocusIfVisible();
        HandleKeyboardShortcuts();
    }
}

void UCmdMsgDlg::SetManager(UCmpnScreen* InManager)
{
    Manager = InManager;
}

void UCmdMsgDlg::ShowMsgDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    bWantsFocus = true;
    bExitLatch = false;
}

void UCmdMsgDlg::HideMsgDlg()
{
    SetVisibility(ESlateVisibility::Hidden);
    bWantsFocus = false;
}

void UCmdMsgDlg::SetTitleText(const FString& InTitle)
{
    if (txt_title)
        txt_title->SetText(FText::FromString(InTitle));
}

void UCmdMsgDlg::SetMessageText(const FString& InMessage)
{
    if (txt_message_rich)
    {
        txt_message_rich->SetText(FText::FromString(InMessage));
    }
    else if (txt_message)
    {
        txt_message->SetText(FText::FromString(InMessage));
    }
}

void UCmdMsgDlg::UpdateFocusIfVisible()
{
    if (!bWantsFocus)
        return;

    // Mimic legacy SetFocus() by focusing the widget (or the close button)
    if (btn_close)
    {
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
        btn_close->SetKeyboardFocus();
    }
    else
    {
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }

    bWantsFocus = false;
}

void UCmdMsgDlg::HandleKeyboardShortcuts()
{
    // Legacy behavior:
    // - Enter closes
    // - Escape closes (with latch so holding Escape does not spam)
    const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
        return;

    // Enter
    if (PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::Virtual_Accept))
    {
        OnCloseClicked();
        return;
    }

    // Escape with latch
    const bool bEscapeDown = PC->IsInputKeyDown(EKeys::Escape);
    if (bEscapeDown)
    {
        if (!bExitLatch)
            OnCloseClicked();

        bExitLatch = true;
    }
    else
    {
        bExitLatch = false;
    }
}

void UCmdMsgDlg::OnCloseClicked()
{
    if (Manager)
    {
        // Legacy: manager->CloseTopmost();
        Manager->CloseTopmost();
    }
    else
    {
        // Safe fallback: just hide
        HideMsgDlg();
    }
}
