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
#include "MenuScreen.h"
#include "AwardPresentation.h"
#include "AudioRouting.h"

// UMG:
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "AwardInfoRegistry.h"
#include "Kismet/GameplayStatics.h"

TSharedRef<SWidget> UMissionAwardDlg::RebuildWidget()
{
    EnsureUi();
    return Super::RebuildWidget();
}

void UMissionAwardDlg::EnsureUi()
{
    if(bModernUiBuilt) return;
    if(!WidgetTree)return;
    if(!RootCanvas)
    {
        RootCanvas=Cast<UCanvasPanel>(GetRootWidget());
        if(!RootCanvas){RootCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RootCanvas"));WidgetTree->RootWidget=RootCanvas;}
    }
    auto* Box=EnsureAutoVerticalBoxWithOffsets(FMargin(24));if(!Box)return;
    if(RootCanvas)for(auto* Child:RootCanvas->GetAllChildren())if(Child!=Box)Child->SetVisibility(ESlateVisibility::Collapsed);
    AwardPresentation::Build(WidgetTree,Box,lbl_name,lbl_info,img_rank,btn_close);
    bModernUiBuilt=true;BindFormWidgets();btn_close->OnClicked.AddUniqueDynamic(this,&UMissionAwardDlg::OnCloseClicked);
}

// Input:
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Starshatter core:
#include "PlayerCharacter.h"
#include "Campaign.h"
#include "Starshatter.h"
#include "SSWRuntimeSubsystem.h"
#include "Sound.h"

UMissionAwardDlg::UMissionAwardDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMissionAwardDlg::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureUi();

    if (btn_close)
        btn_close->OnClicked.AddUniqueDynamic(this, &UMissionAwardDlg::OnCloseClicked);

}

FReply UMissionAwardDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept ||
        Key == EKeys::Escape || Key == EKeys::Virtual_Back)
    {
        if (!InKeyEvent.IsRepeat())
            OnCloseClicked();

        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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
      fore_color:    (255,255,255)
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
    EnsureUi();
    SetVisibility(ESlateVisibility::Visible);
    if (UTextBlock* TitleBlock = Cast<UTextBlock>(WidgetTree ? WidgetTree->FindWidget(TEXT("PanelTitle")) : nullptr))
        TitleBlock->SetText(FText::FromString(TEXT("Congratulations")));
    ShowPlayer();
    SetIsFocusable(true);
    SetKeyboardFocus();
}

void UMissionAwardDlg::ShowPlayer()
{
    PlayerCharacter* P = PlayerCharacter::GetCurrentPlayer();

    if (P)
    {
        if (lbl_name)
        {
            lbl_name->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            lbl_name->SetText(FText::FromString(P->AwardName()));
        }

        if (lbl_info)
        {
            lbl_info->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            lbl_info->SetText(FText::FromString(P->AwardDesc()));
        }

        if (img_rank)
        {
            img_rank->SetVisibility(ESlateVisibility::Visible);
            const int32 PendingId = P->GetPendingAwardId();
            const bool bPendingIsRank = P->IsPendingAwardRank();
            const FMedalInfo* Medal = bPendingIsRank ? nullptr : UAwardInfoRegistry::FindMedal(PendingId);
            const FRankInfo* RankInfo = bPendingIsRank ? UAwardInfoRegistry::FindRank(PendingId) : nullptr;
            UTexture2D* Tex = nullptr;
            if (RankInfo && RankInfo->LargeImage.ToSoftObjectPath().IsValid())
                Tex = RankInfo->LargeImage.LoadSynchronous();
            else if (Medal && Medal->LargeImage.ToSoftObjectPath().IsValid())
                Tex = Medal->LargeImage.LoadSynchronous();
            if (!Tex)
            {
                const FString Award = P->AwardName();
                if (!Award.IsEmpty())
                    Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/UI/Awards/%s.%s"), *Award, *Award));
            }
            if (Tex) img_rank->SetBrushFromTexture(Tex);

            USoundBase* Sound = nullptr;
            if (RankInfo && RankInfo->GrantSound.ToSoftObjectPath().IsValid())
                Sound = RankInfo->GrantSound.LoadSynchronous();
            else if (Medal && Medal->GranSound.ToSoftObjectPath().IsValid())
                Sound = Medal->GranSound.LoadSynchronous();
            if (Sound) SSWAudioRouting::Play2D(this,Sound,EStarshatterAudioChannel::Voice);
        }
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
    if (PlayerCharacter* P = PlayerCharacter::GetCurrentPlayer())
    {
        P->ClearShowAward();
    }

    SetVisibility(ESlateVisibility::Collapsed);

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("AwardDlg: GameInstance not found."));
        return;
    }

    USSWRuntimeSubsystem* RuntimeSS = GI->GetSubsystem<USSWRuntimeSubsystem>();
    if (!RuntimeSS)
    {
        UE_LOG(LogTemp, Warning, TEXT("AwardDlg: Runtime subsystem not found."));
        return;
    }

    Campaign* Cmpn = Campaign::GetCampaign();
    if (Cmpn && Cmpn->GetCampaignId() < Campaign::SINGLE_MISSIONS)
    {
        RuntimeSS->SetGameMode(EGameMode::CMPN);
    }
    else
    {
        RuntimeSS->SetGameMode(EGameMode::MENU);
    }

    if (UMenuScreen* Menu = GetMenuManager())
        Menu->ReturnFromMissionBriefing();
}