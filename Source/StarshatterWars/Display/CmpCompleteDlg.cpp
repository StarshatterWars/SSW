/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmpCompleteDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpCompleteDlg implementation.
*/

#include "CmpCompleteDlg.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#include "Campaign.h"
#include "CombatEvent.h"
#include "CmpnScreen.h"

UCmpCompleteDlg::UCmpCompleteDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpCompleteDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildScreen();

    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UCmpCompleteDlg::HandleCloseClicked);
        CloseButton->OnClicked.AddDynamic(this, &UCmpCompleteDlg::HandleCloseClicked);
    }

    SetVisibility(ESlateVisibility::Collapsed);
    SetDialogInputEnabled(false);
}

void UCmpCompleteDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (GetVisibility() == ESlateVisibility::Visible)
    {
        ShowTime += InDeltaTime;
    }
}

void UCmpCompleteDlg::BuildScreen()
{
    if (bScreenBuilt || !WidgetTree)
    {
        return;
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    BuildTopBackground();
    BuildCenterBanner();
    BuildBottomPanel();

    bScreenBuilt = true;
}

void UCmpCompleteDlg::BuildTopBackground()
{
    BgTop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BgTop"));

    UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(BgTop);
    if (TopSlot)
    {
        TopSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
        TopSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 120.f));
        TopSlot->SetZOrder(0);
    }

    BgTop->SetColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.04f, 1.0f));
}

void UCmpCompleteDlg::BuildCenterBanner()
{
    TitleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleImage"));

    UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(TitleImage);
    if (CenterSlot)
    {
        CenterSlot->SetAnchors(FAnchors(0.5f, 0.45f, 0.5f, 0.45f));
        CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CenterSlot->SetSize(FVector2D(1024.f, 512.f));
        CenterSlot->SetPosition(FVector2D(0.f, 0.f));
        CenterSlot->SetZOrder(10);
    }
}

void UCmpCompleteDlg::BuildBottomPanel()
{
    BgBottom = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BgBottom"));

    UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(BgBottom);
    if (BgSlot)
    {
        BgSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
        BgSlot->SetAlignment(FVector2D(0.f, 1.f));
        BgSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 140.f));
        BgSlot->SetZOrder(1);
    }

    BgBottom->SetColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.08f, 0.95f));

    InfoLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoLabel"));
    UCanvasPanelSlot* InfoSlot = RootCanvas->AddChildToCanvas(InfoLabel);
    if (InfoSlot)
    {
        InfoSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
        InfoSlot->SetAlignment(FVector2D(0.5f, 1.f));
        InfoSlot->SetPosition(FVector2D(0.f, -72.f));
        InfoSlot->SetSize(FVector2D(600.f, 32.f));
        InfoSlot->SetZOrder(20);
    }

    InfoLabel->SetText(FText::FromString(TEXT("CAMPAIGN COMPLETE")));
    InfoLabel->SetJustification(ETextJustify::Center);

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
    UCanvasPanelSlot* ButtonSlot = RootCanvas->AddChildToCanvas(CloseButton);
    if (ButtonSlot)
    {
        ButtonSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
        ButtonSlot->SetAlignment(FVector2D(0.5f, 1.f));
        ButtonSlot->SetPosition(FVector2D(0.f, -20.f));
        ButtonSlot->SetSize(FVector2D(180.f, 36.f));
        ButtonSlot->SetZOrder(30);
    }

    CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
    CloseButtonText->SetText(FText::FromString(TEXT("CLOSE")));
    CloseButtonText->SetJustification(ETextJustify::Center);
    CloseButton->AddChild(CloseButtonText);
}

void UCmpCompleteDlg::ShowCompleteDlg()
{
    ShowTime = 0.0f;
    CampaignPtr = Campaign::GetCampaign();

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
    SetIsFocusable(true);
    SetDialogInputEnabled(true);

    if (!CampaignPtr || !TitleImage)
    {
        return;
    }

    CombatEvent* Event = CampaignPtr->GetLastEvent();
    if (!Event)
    {
        return;
    }

    FString ImageName = UTF8_TO_TCHAR(Event->ImageFile());
    FString CampaignPath = UTF8_TO_TCHAR(CampaignPtr->Path());

    if (ImageName.IsEmpty() || CampaignPath.IsEmpty())
    {
        return;
    }

    if (!ImageName.EndsWith(TEXT(".pcx"), ESearchCase::IgnoreCase))
    {
        ImageName += TEXT(".pcx");
    }

    BannerTexture = LoadCampaignTexture(CampaignPath, ImageName);
    if (BannerTexture)
    {
        TitleImage->SetBrushFromTexture(BannerTexture, false);
    }
}

void UCmpCompleteDlg::HideCompleteDlg()
{
    SetDialogInputEnabled(false);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UCmpCompleteDlg::HandleCloseClicked()
{
    HideCompleteDlg();

    if (Manager)
    {
        Manager->ShowCmdDlg();
    }
}

UTexture2D* UCmpCompleteDlg::LoadCampaignTexture(const FString& CampaignPath, const FString& ImageFile) const
{
    // Stub for now.
    // Hook your DataLoader/bitmap bridge here later.
    (void)CampaignPath;
    (void)ImageFile;
    return nullptr;
}