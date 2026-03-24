/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         AwardDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UAwardDlg implementation (Unreal port)
*/

#include "MissionAwardDlg.h"

// UMG:
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

// Input:
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Starshatter core:
#include "PlayerCharacter.h"
#include "Campaign.h"
#include "Starshatter.h"

// If you have a sound wrapper, include it here.
// #include "Sound.h"

UMissionAwardDlg::UMissionAwardDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionAwardDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind close:
    if (btn_close)
    {
        btn_close->OnClicked.AddDynamic(this, &UMissionAwardDlg::OnCloseClicked);
    }

    // Match legacy behavior: first frame is latched so Enter/Escape doesn't instantly close.
    bExitLatch = true;

    // When shown, the owning screen typically calls ShowDialog().
    // We do not auto-call ShowPlayer() here because some screens construct hidden.
}

FReply UMissionAwardDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Mirror legacy behavior:
    // - Enter/Escape close only if latch is released.
    const FKey Key = InKeyEvent.GetKey();

    if ((Key == EKeys::Enter || Key == EKeys::Virtual_Accept ||
        Key == EKeys::Escape || Key == EKeys::Virtual_Back))
    {
        bool bHandled = false;
        UpdateExitLatchFromInput(InKeyEvent, bHandled);

        if (bHandled)
            return FReply::Handled();

        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }

    // Any other key releases the latch in the classic code only after keys are up;
    // here we keep it simple: first non-Enter/Escape keydown releases latch.
    bExitLatch = false;

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMissionAwardDlg::UpdateExitLatchFromInput(const FKeyEvent& InKeyEvent, bool& bOutHandled)
{
    bOutHandled = false;

    const FKey Key = InKeyEvent.GetKey();

    const bool bIsAccept = (Key == EKeys::Enter || Key == EKeys::Virtual_Accept);
    const bool bIsBack = (Key == EKeys::Escape || Key == EKeys::Virtual_Back);

    if (!bIsAccept && !bIsBack)
        return;

    if (bExitLatch)
    {
        // First frame / first key press while latched: do nothing, just release latch.
        bExitLatch = false;
        bOutHandled = true;
        return;
    }

    // Close once latch is released:
    OnCloseClicked();
    bOutHandled = true;
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UMissionAwardDlg::BindFormWidgets()
{
    // Bind FORM IDs to widgets (IDs come from AwardDlg.frm)
    BindLabel(203, lbl_name);
    BindLabel(201, lbl_info);
    BindImage(202, img_rank);
    BindButton(1, btn_close);
}

FString UMissionAwardDlg::GetLegacyFormText() const
{
    // Raw FORM text (AwardDlg.frm)
    // NOTE: If you keep the /*** ... ***/ commented block, ensure BaseScreen.cpp
    // strips block comments (see prior note).
    return TEXT(R"FORM(
form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10,  50, 512,  50, 10)
      x_weights:  ( 0,   1,   0,   1,  0)

      y_mins:     (28, 25, 20,  5, 30, 256, 10, 50, 45)
      y_weights:  ( 0,  0,  0,  1,  0,   0,  0,  2,  0)
   },

   /***
   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,4,6),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (5,3,3,6),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }
   ***/

   ctrl: {
      id:            10,
      type:          label,
      text:          "Congratulations",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,0)
   },

   ctrl: {
      id:            203,
      type:          label,
      cells:         (2,4,1,1)
      align:         center
      transparent:   true
      border:        false
      font:          Limerick18
      fore_color:    (255,255,255)
      back_color:    ( 10, 10, 10)
      style:         0x0040
   },

   ctrl: {
      id:            202,
      type:          image,
      cells:         (2,5,1,1)
      align:         center
      transparent:   true
      border:        false
   },

   ctrl: {
      id:            201,
      type:          label,
      cells:         (2,7,1,1)
      align:         left
      transparent:   true
      border:        false
      font:          Verdana
      fore_color:    (0,0,0)
   },

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (50,5,0,0)
      fixed_height:     19
   },

   ctrl: {
      id:            1
      pid:           0
      type:          button
      text:          Close
      cells:         (3,8,1,1),
   },

}
)FORM");
}

// --------------------------------------------------------------------
// Public API (ported behavior)
// --------------------------------------------------------------------

void UMissionAwardDlg::ShowDialog()
{
    // Legacy Show(): show window, then ShowPlayer(), and latch input.
    SetVisibility(ESlateVisibility::Visible);

    ShowPlayer();

    bExitLatch = true;
}

void UMissionAwardDlg::ShowPlayer()
{
    PlayerCharacter* P = PlayerCharacter::GetCurrentPlayer();

    if (P)
    {
        if (lbl_name)
            lbl_name->SetText(FText::FromString(P->AwardName()));

        if (lbl_info)
            lbl_info->SetText(FText::FromString(P->AwardDesc()));

        if (img_rank)
        {
            // NOTE:
            // Legacy: img_rank->SetPicture(*p->AwardImage());
            // Unreal: you need a UTexture2D/brush pipeline.
            // This is intentionally left minimal and non-assumptive.
            img_rank->SetVisibility(ESlateVisibility::Visible);
        }

        // Legacy: play award sound
        // if (USound* Congrats = P->AwardSound()) Congrats->Play();
    }
    else
    {
        if (lbl_info)
            lbl_info->SetText(FText::GetEmpty());

        if (img_rank)
            img_rank->SetVisibility(ESlateVisibility::Hidden);
    }
}

// --------------------------------------------------------------------
// Actions
// --------------------------------------------------------------------

void UMissionAwardDlg::OnCloseClicked()
{
    // Legacy behavior:
    // Player::ClearShowAward(); then switch Starshatter mode based on campaign id.

    if (PlayerCharacter* P = PlayerCharacter::GetCurrentPlayer())
    {
        P->ClearShowAward();
    }

    Starshatter* Stars = Starshatter::GetInstance();
    if (!Stars)
    {
        UE_LOG(LogTemp, Warning, TEXT("AwardDlg: Starshatter instance not found."));
        SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    // Legacy hides mouse:
    // Mouse::Show(false);
    // (Implement in your input layer if needed.)

    Campaign* Cmpn = Campaign::GetCampaign();
    if (Cmpn && Cmpn->GetCampaignId() < Campaign::SINGLE_MISSIONS)
        Stars->SetGameMode(EGameMode::CMPN);
    else
        Stars->SetGameMode(EGameMode::MENU);

    // Close/hide widget:
    SetVisibility(ESlateVisibility::Hidden);
}
