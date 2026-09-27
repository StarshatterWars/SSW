/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         ConfirmDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
*/

#include "ConfirmDlg.h"

// UMG:
#include "Components/Button.h"
#include "Components/TextBlock.h"

// Manager:
#include "MenuScreen.h" // your UE port class header that defines UMenuScreen

UConfirmDlg::UConfirmDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Ensure UBaseScreen routes input:
    SetDialogInputEnabled(true);
}

void UConfirmDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RegisterControls();
}

void UConfirmDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

void UConfirmDlg::RegisterControls()
{
    // UBaseScreen expects these for centralized Enter/Escape:
    ApplyButton = btn_apply;
    CancelButton = btn_cancel;

    if (btn_apply)
    {
        btn_apply->OnClicked.RemoveAll(this);
        btn_apply->OnClicked.AddDynamic(this, &UConfirmDlg::HandleApplyClicked);
    }

    if (btn_cancel)
    {
        btn_cancel->OnClicked.RemoveAll(this);
        btn_cancel->OnClicked.AddDynamic(this, &UConfirmDlg::HandleCancelClicked);
    }
}

void UConfirmDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    // Nice-to-have: focus so Enter/Escape works immediately
    SetKeyboardFocus();
}

// ---------------------------------------------------------------------
// UBaseScreen centralized input

void UConfirmDlg::HandleAccept()
{
    HandleApplyClicked();
}

void UConfirmDlg::HandleCancel()
{
    HandleCancelClicked();
}

// ---------------------------------------------------------------------
// Click handlers

void UConfirmDlg::HandleApplyClicked()
{
    if (Manager)
    {
        Manager->HideConfirmDlg();
    }

    OnConfirmed.Broadcast();
}

void UConfirmDlg::HandleCancelClicked()
{
    if (Manager)
    {
        Manager->HideConfirmDlg();
    }

    OnCanceled.Broadcast();
}

// ---------------------------------------------------------------------
// Text API

FString UConfirmDlg::GetTitle() const
{
    return lbl_title ? lbl_title->GetText().ToString() : FString();
}

void UConfirmDlg::SetTitle(const FString& InTitle)
{
    if (lbl_title)
        lbl_title->SetText(FText::FromString(InTitle));
}

FString UConfirmDlg::GetDialogMessage() const
{
    return lbl_message ? lbl_message->GetText().ToString() : FString();
}

void UConfirmDlg::SetMessage(const FString& InMessage)
{
    if (lbl_message)
        lbl_message->SetText(FText::FromString(InMessage));
}

// ---------------------------------------------------------------------
// UBaseScreen overrides

void UConfirmDlg::BindFormWidgets()
{
    // FORM id -> widget mapping
    if (btn_apply)  BindButton(1, btn_apply);
    if (btn_cancel) BindButton(2, btn_cancel);

    if (lbl_title)   BindLabel(100, lbl_title);
    if (lbl_message) BindLabel(101, lbl_message);

    // Ensure UBaseScreen uses these:
    ApplyButton = btn_apply;
    CancelButton = btn_cancel;
}

FString UConfirmDlg::GetLegacyFormText() const
{
    return TEXT(R"FORM(
form: {
   rect:       (0,0,400,280)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 40, 100, 100, 20),
      x_weights:  ( 0,  0,   1,   1,  0),

      y_mins:     (44,  30, 30, 80, 35),
      y_weights:  ( 0,   0,  0,  1,  0)
   }

   defctrl: {
      fore_color:    (255, 255, 255)
      font:          Limerick12
      transparent:   true
   }

   ctrl: {
      id:            100
      type:          label
      text:          "Are You Sure?"
      align:         center
      cells:         (1,1,3,1)
      font:          Limerick18
   }

   ctrl: {
      id:            101
      type:          label
      text:          "Are you sure you want to take this action?"
      align:         left
      font:          Verdana
      cells:         (1,2,3,1)
   }

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,16)
      transparent:      false
   }

   ctrl: {
      id:            1,
      type:          button,
      cells:         (2,4,1,1)
      text:          "OK"
   }

   ctrl: {
      id:            2,
      type:          button,
      cells:         (3,4,1,1)
      text:          "Cancel"
   }
}
)FORM");
}
