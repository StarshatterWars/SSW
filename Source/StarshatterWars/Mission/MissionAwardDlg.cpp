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

void UMissionAwardDlg::EnsureUi()
{
    if (!WidgetTree) return;

    USizeBox* Host = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("RuntimeHost")));
    if (!Host)
    {
        UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
        if (!Root)
        {
            Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AwardRoot"));
            WidgetTree->RootWidget = Root;
        }

        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
        if (UTexture2D* FrameTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Frame1.Frame1")))
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(FrameTex);
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.Margin = FMargin(0.2f, 0.2f, 0.2f, 0.25f);
            Frame->SetBrush(Brush);
        }
        if (UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(Frame))
        {
            FrameSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            FrameSlot->SetOffsets(FMargin(0.f));
        }

        UCanvasPanel* Inner = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AwardInner"));
        Frame->AddChild(Inner);

        UTextBlock* TitleBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PanelTitle"));
        TitleBlock->SetText(FText::FromString(TEXT("Congratulations")));
        TitleBlock->SetJustification(ETextJustify::Center);
        TitleBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        if (UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font")))
        {
            FSlateFontInfo Info = TitleBlock->GetFont();
            Info.FontObject = Font;
            Info.Size = 22;
            TitleBlock->SetFont(Info);
        }
        if (UCanvasPanelSlot* TitleSlot = Inner->AddChildToCanvas(Title))
        {
            TitleSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
            TitleSlot->SetOffsets(FMargin(0.f, 32.f, 0.f, 28.f));
            TitleSlot->SetAlignment(FVector2D(0.f, 0.f));
        }

        Host = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RuntimeHost"));
        if (UCanvasPanelSlot* HostSlot = Inner->AddChildToCanvas(Host))
        {
            HostSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            HostSlot->SetOffsets(FMargin(48.f, 72.f, 48.f, 24.f));
        }
    }

    if (lbl_name && lbl_info && img_rank && btn_close) return;

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AwardContent"));
    Host->AddChild(Box);

    auto AddLabel = [this, Box](const TCHAR* Name, int32 Size) -> UTextBlock*
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(true);
        if (UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font")))
        {
            FSlateFontInfo Info = Label->GetFont();
            Info.FontObject = Font;
            Info.Size = Size;
            Label->SetFont(Info);
        }
        if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Label))
        {
            Slot->SetPadding(FMargin(12.f, 8.f));
            Slot->SetHorizontalAlignment(HAlign_Center);
        }
        return Label;
    };

    if (!lbl_name) lbl_name = AddLabel(TEXT("AwardName"), 20);
    if (!img_rank)
    {
        img_rank = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AwardImage"));
        USizeBox* Sized = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AwardImageSize"));
        Sized->SetHeightOverride(160.f);
        Sized->SetWidthOverride(160.f);
        Sized->AddChild(img_rank);
        if (UVerticalBoxSlot* ImageSlot = Box->AddChildToVerticalBox(Sized))
            ImageSlot->SetHorizontalAlignment(HAlign_Center);
    }
    if (!lbl_info) lbl_info = AddLabel(TEXT("AwardInfo"), 14);
    if (!btn_close)
    {
        btn_close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AwardClose"));
        UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AwardCloseLabel"));
        CloseLabel->SetText(FText::FromString(TEXT("Close")));
        CloseLabel->SetJustification(ETextJustify::Center);
        btn_close->AddChild(CloseLabel);
        if (UVerticalBoxSlot* CloseSlot = Box->AddChildToVerticalBox(btn_close))
        {
            CloseSlot->SetPadding(FMargin(0.f, 16.f));
            CloseSlot->SetHorizontalAlignment(HAlign_Center);
        }
    }

    BindFormWidgets();
    if (btn_close)
        btn_close->OnClicked.AddUniqueDynamic(this, &UMissionAwardDlg::OnCloseClicked);
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

    bExitLatch = true;
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
    bExitLatch = true;
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

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("AwardDlg: GameInstance not found."));
        SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    USSWRuntimeSubsystem* RuntimeSS = GI->GetSubsystem<USSWRuntimeSubsystem>();
    if (!RuntimeSS)
    {
        UE_LOG(LogTemp, Warning, TEXT("AwardDlg: Runtime subsystem not found."));
        SetVisibility(ESlateVisibility::Hidden);
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

    SetVisibility(ESlateVisibility::Hidden);
}