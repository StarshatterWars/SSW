/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpLoadDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Code-built campaign loading dialog.

    This replaces the legacy FORM-driven CmpLoadDlg layout with a
    native Unreal UMG screen built entirely in C++.

    Visual layout:
    - Full-screen background image (starfield fallback tint if missing)
    - Centered scrCampaignLoad texture
    - Campaign name centered over the art in large Serpentine font
    - Bottom panel with loading activity text and progress bar

    Behavior parity:
    - Show() captures display time
    - ExecFrame() refreshes activity/progress
    - IsDone() enforces a 5 second minimum display duration
    - When complete, transitions to UCmpnScreen once
*/



#include "CmpLoadDlg.h"

// UMG:
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// Engine:
#include "Engine/Texture2D.h"
#include "Fonts/SlateFontInfo.h"
#include "HAL/PlatformTime.h"
#include "Math/Color.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

// Starshatter:
#include "Campaign.h"
#include "Starshatter.h"
#include "MenuScreen.h"

UCmpLoadDlg::UCmpLoadDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpLoadDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildScreen();
    LoadArtAssets();
    ApplyStaticArt();
    ApplyTitleFont();
    ApplyInitialVisualState();
    ApplyCampaignTitleCard();
    RefreshLoadState();

    SetVisibility(ESlateVisibility::Hidden);
    SetDialogInputEnabled(false);
}

void UCmpLoadDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() == ESlateVisibility::Visible)
    {
        ExecFrame((double)InDeltaTime);
    }
}

void UCmpLoadDlg::Show()
{
    bTransitionedToCmpnScreen = false;

    BuildScreen();
    LoadArtAssets();
    ApplyStaticArt();
    ApplyTitleFont();
    ApplyCampaignTitleCard();
    ApplyInitialVisualState();
    RefreshLoadState();

    ShowTimeMs = GetRealTimeMs();

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
    SetIsFocusable(true);
    SetDialogInputEnabled(true);
}

void UCmpLoadDlg::Hide()
{
    SetDialogInputEnabled(false);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCmpLoadDlg::ExecFrame(double DeltaTime)
{
    (void)DeltaTime;

    RefreshLoadState();

    if (!bTransitionedToCmpnScreen && IsDone())
    {
        bTransitionedToCmpnScreen = true;

        if (MenuManager)
        {
            MenuManager->ShowCmpnScreen();
        }
    }
}

bool UCmpLoadDlg::IsDone() const
{
    const uint32 Now = GetRealTimeMs();

    if (Now - ShowTimeMs < 5000u)
    {
        return false;
    }

    return true;
}

void UCmpLoadDlg::BuildScreen()
{
    if (bScreenBuilt || !WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    BuildBackgroundLayer();
    BuildMainLayout();

    bScreenBuilt = true;
}

void UCmpLoadDlg::BuildBackgroundLayer()
{
    if (!RootCanvas || !WidgetTree)
    {
        return;
    }

    BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BackgroundImage"));

    if (UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(BackgroundImage))
    {
        BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        BgSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        BgSlot->SetAlignment(FVector2D(0.f, 0.f));
        BgSlot->SetAutoSize(false);
        BgSlot->SetZOrder(0);
    }
}

void UCmpLoadDlg::BuildMainLayout()
{
    if (!RootCanvas || !WidgetTree)
    {
        return;
    }

    MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));

    if (UCanvasPanelSlot* MainSlot = RootCanvas->AddChildToCanvas(MainVBox))
    {
        MainSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        MainSlot->SetOffsets(FMargin(36.f, 24.f, 36.f, 28.f));
        MainSlot->SetAlignment(FVector2D(0.f, 0.f));
        MainSlot->SetAutoSize(false);
        MainSlot->SetZOrder(10);
    }

    BuildVBoxSpacer(MainVBox, 24.f);

    CenterArtBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CenterArtBox"));
    CenterArtBox->SetWidthOverride(1024.f);
    CenterArtBox->SetHeightOverride(512.f);

    if (UVerticalBoxSlot* CenterArtSlot = MainVBox->AddChildToVerticalBox(CenterArtBox))
    {
        CenterArtSlot->SetHorizontalAlignment(HAlign_Center);
        CenterArtSlot->SetVerticalAlignment(VAlign_Center);
        CenterArtSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
        CenterArtSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    CenterArtOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CenterArtOverlay"));
    CenterArtBox->AddChild(CenterArtOverlay);

    CenterImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CenterImage"));
    if (UOverlaySlot* ImageSlot = CenterArtOverlay->AddChildToOverlay(CenterImage))
    {
        ImageSlot->SetHorizontalAlignment(HAlign_Fill);
        ImageSlot->SetVerticalAlignment(VAlign_Fill);
        ImageSlot->SetPadding(FMargin(0.f));
    }

    CenterTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterTitleText"));
    if (UOverlaySlot* TitleSlot = CenterArtOverlay->AddChildToOverlay(CenterTitleText))
    {
        TitleSlot->SetHorizontalAlignment(HAlign_Center);
        TitleSlot->SetVerticalAlignment(VAlign_Center);
        TitleSlot->SetPadding(FMargin(96.f, 0.f, 96.f, 0.f));
    }

    CenterTitleText->SetText(FText::FromString(TEXT("CAMPAIGN")));
    CenterTitleText->SetJustification(ETextJustify::Center);
    CenterTitleText->SetAutoWrapText(true);
    CenterTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.45f, 1.0f)));
    CenterTitleText->SetShadowOffset(FVector2D(2.f, 2.f));
    CenterTitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.90f));

    BuildBottomPanel();
}

void UCmpLoadDlg::BuildBottomPanel()
{
    if (!MainVBox || !WidgetTree)
    {
        return;
    }

    BottomPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BottomPanel"));
    BottomPanel->SetBrushColor(FLinearColor(0.06f, 0.07f, 0.09f, 0.92f));
    BottomPanel->SetPadding(FMargin(24.f, 18.f, 24.f, 18.f));

    if (UVerticalBoxSlot* BottomSlot = MainVBox->AddChildToVerticalBox(BottomPanel))
    {
        BottomSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BottomSlot->SetHorizontalAlignment(HAlign_Fill);
        BottomSlot->SetVerticalAlignment(VAlign_Bottom);
        BottomSlot->SetPadding(FMargin(0.f));
    }

    BottomPanelVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BottomPanelVBox"));
    BottomPanel->SetContent(BottomPanelVBox);

    LblActivity = BuildVBoxLabel(
        BottomPanelVBox,
        TEXT("LOADING..."),
        18,
        FLinearColor(0.86f, 0.89f, 0.93f, 1.0f));

    if (LblActivity)
    {
        LblActivity->SetJustification(ETextJustify::Center);
    }

    BuildVBoxSpacer(BottomPanelVBox, 14.f);

    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar"));
    ProgressBar->SetPercent(0.0f);
    ProgressBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.94f, 0.55f, 1.0f));

    if (UVerticalBoxSlot* ProgressSlot = BottomPanelVBox->AddChildToVerticalBox(ProgressBar))
    {
        ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
        ProgressSlot->SetVerticalAlignment(VAlign_Center);
        ProgressSlot->SetPadding(FMargin(40.f, 0.f, 40.f, 0.f));
    }
}

void UCmpLoadDlg::LoadArtAssets()
{
    if (!DefaultCenterTexture)
    {
        DefaultCenterTexture = LoadObject<UTexture2D>(
            nullptr,
            TEXT("/Game/UI/scrCampaignLoad.scrCampaignLoad"));
    }

    if (!DefaultBackgroundTexture)
    {
        DefaultBackgroundTexture = LoadObject<UTexture2D>(
            nullptr,
            TEXT("/Game/UI/T_StarfieldBackground.T_StarfieldBackground"));
    }

    if (!SerpentineFontObject)
    {
        SerpentineFontObject = LoadObject<UObject>(
            nullptr,
            TEXT("/Game/Font/SERPNTB_Font.SERPNTB_Font"));
    }
}

void UCmpLoadDlg::ApplyStaticArt()
{
    if (BackgroundImage)
    {
        if (DefaultBackgroundTexture)
        {
            FSlateBrush BgBrush;
            BgBrush.SetResourceObject(DefaultBackgroundTexture);
            BgBrush.ImageSize = FVector2D(
                (float)DefaultBackgroundTexture->GetSizeX(),
                (float)DefaultBackgroundTexture->GetSizeY());

            BackgroundImage->SetBrush(BgBrush);
            BackgroundImage->SetColorAndOpacity(FLinearColor(0.34f, 0.34f, 0.38f, 1.0f));
        }
        else
        {
            BackgroundImage->SetColorAndOpacity(FLinearColor(0.01f, 0.01f, 0.03f, 1.0f));
        }
    }

    if (CenterImage && DefaultCenterTexture)
    {
        FSlateBrush CenterBrush;
        CenterBrush.SetResourceObject(DefaultCenterTexture);
        CenterBrush.ImageSize = FVector2D(
            (float)DefaultCenterTexture->GetSizeX(),
            (float)DefaultCenterTexture->GetSizeY());

        CenterImage->SetBrush(CenterBrush);
        CenterImage->SetColorAndOpacity(FLinearColor::White);
    }
}

void UCmpLoadDlg::ApplyTitleFont()
{
    if (!CenterTitleText)
    {
        return;
    }

    FSlateFontInfo FontInfo = CenterTitleText->GetFont();
    FontInfo.Size = 48;

    if (SerpentineFontObject)
    {
        FontInfo.FontObject = SerpentineFontObject;
    }

    CenterTitleText->SetFont(FontInfo);
}

void UCmpLoadDlg::ApplyCampaignTitleCard()
{
    FString CampaignName = TEXT("CAMPAIGN");

    Campaign* CampaignObj = Campaign::GetCampaign();
    if (CampaignObj && CampaignObj->GetName())
    {
        CampaignName = UTF8_TO_TCHAR(CampaignObj->GetName());
    }

    if (CenterTitleText)
    {
        CenterTitleText->SetText(FText::FromString(CampaignName));
    }

    if (CenterImage && DefaultCenterTexture)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(DefaultCenterTexture);
        Brush.ImageSize = FVector2D(
            (float)DefaultCenterTexture->GetSizeX(),
            (float)DefaultCenterTexture->GetSizeY());

        CenterImage->SetBrush(Brush);
    }
}

void UCmpLoadDlg::ApplyInitialVisualState()
{
    if (LblActivity)
    {
        LblActivity->SetText(FText::FromString(TEXT("LOADING...")));
    }

    if (ProgressBar)
    {
        ProgressBar->SetPercent(0.0f);
    }
}

void UCmpLoadDlg::RefreshLoadState()
{
    Starshatter* Stars = Starshatter::GetInstance();
    if (!Stars)
    {
        if (LblActivity)
        {
            LblActivity->SetText(FText::FromString(TEXT("INITIALIZING...")));
        }

        if (ProgressBar)
        {
            ProgressBar->SetPercent(0.0f);
        }

        return;
    }

    if (LblActivity)
    {
        const char* Activity = Stars->GetLoadActivity();
        const FString ActivityText = Activity ? UTF8_TO_TCHAR(Activity) : TEXT("LOADING...");
        LblActivity->SetText(FText::FromString(ActivityText));
    }

    if (ProgressBar)
    {
        const float Progress = FMath::Clamp((float)Stars->GetLoadProgress(), 0.0f, 1.0f);
        ProgressBar->SetPercent(Progress);
    }
}

uint32 UCmpLoadDlg::GetRealTimeMs() const
{
    const double Sec = FPlatformTime::Seconds();
    return (uint32)(Sec * 1000.0);
}

UTextBlock* UCmpLoadDlg::BuildVBoxLabel(
    UVerticalBox* Parent,
    const FString& Text,
    int32 FontSize,
    const FLinearColor& Color)
{
    if (!Parent || !WidgetTree)
    {
        return nullptr;
    }

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    if (!Label)
    {
        return nullptr;
    }

    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetAutoWrapText(true);

    FSlateFontInfo FontInfo = Label->GetFont();
    FontInfo.Size = FontSize;
    Label->SetFont(FontInfo);

    if (UVerticalBoxSlot* VSlot = Parent->AddChildToVerticalBox(Label))
    {
        VSlot->SetHorizontalAlignment(HAlign_Fill);
        VSlot->SetVerticalAlignment(VAlign_Center);
        VSlot->SetPadding(FMargin(0.f));
    }

    return Label;
}

USpacer* UCmpLoadDlg::BuildVBoxSpacer(UVerticalBox* Parent, float Height)
{
    if (!Parent || !WidgetTree)
    {
        return nullptr;
    }

    USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
    if (!Spacer)
    {
        return nullptr;
    }

    Spacer->SetSize(FVector2D(1.f, Height));

    if (UVerticalBoxSlot* VSlot = Parent->AddChildToVerticalBox(Spacer))
    {
        VSlot->SetHorizontalAlignment(HAlign_Fill);
        VSlot->SetVerticalAlignment(VAlign_Center);
        VSlot->SetPadding(FMargin(0.f));
        VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    return Spacer;
}