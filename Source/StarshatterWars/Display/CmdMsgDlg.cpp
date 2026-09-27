/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI
    FILE:         CmdMsgDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Campaign message dialog (modal).
    Handles input (mouse, Enter, Escape) and routes closing
    through UCmpnScreen to preserve UI flow.
*/

#include "CmdMsgDlg.h"
#include "CmpnScreen.h"

#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

// +-------------------------------------------------------------------+

UCmdMsgDlg::UCmdMsgDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::NativeConstruct()
{
    Super::NativeConstruct();

    bExitLatch = false;
    bWantsFocus = false;

    SetIsFocusable(true);
    HideMsgDlg();
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() == ESlateVisibility::Visible)
    {
        UpdateFocusIfVisible();
        HandleKeyboardShortcuts();
    }
}

// +-------------------------------------------------------------------+

FReply UCmdMsgDlg::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (GetVisibility() == ESlateVisibility::Visible &&
        InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (CmpnScreen)
        {
            CmpnScreen->HideCmdMsgDlg();
        }
        else
        {
            HideMsgDlg();
        }

        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::ShowMsgDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[CmdMsgDlg] ShowMsgDlg"));

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);

    bWantsFocus = true;
    bExitLatch = false;
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::HideMsgDlg()
{
    SetVisibility(ESlateVisibility::Hidden);
    SetIsEnabled(false);

    bWantsFocus = false;
    bExitLatch = false;
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::SetTitleText(const FString& InTitle)
{
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(InTitle));
    }
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::SetMessageText(const FString& InMessage)
{
    if (MessageTextBlock)
    {
        MessageTextBlock->SetText(FText::FromString(InMessage));
    }
    else if (MessageText)
    {
        MessageText->SetText(FText::FromString(InMessage));
    }
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::UpdateFocusIfVisible()
{
    if (!bWantsFocus)
    {
        return;
    }

    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    SetKeyboardFocus();

    bWantsFocus = false;
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::HandleKeyboardShortcuts()
{
    const APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        return;
    }

    // ENTER / ACCEPT
    if (PC->WasInputKeyJustPressed(EKeys::Enter) ||
        PC->WasInputKeyJustPressed(EKeys::Virtual_Accept))
    {
        if (CmpnScreen)
        {
            CmpnScreen->HideCmdMsgDlg();
        }
        else
        {
            HideMsgDlg();
        }

        return;
    }

    // ESCAPE
    const bool bEscapeDown = PC->IsInputKeyDown(EKeys::Escape);
    if (bEscapeDown)
    {
        if (!bExitLatch)
        {
            if (CmpnScreen)
            {
                CmpnScreen->HideCmdMsgDlg();
            }
            else
            {
                HideMsgDlg();
            }
        }

        bExitLatch = true;
    }
    else
    {
        bExitLatch = false;
    }
}

// +-------------------------------------------------------------------+

void UCmdMsgDlg::SetCmpnScreen(UCmpnScreen* InScreen)
{
    CmpnScreen = InScreen;
}