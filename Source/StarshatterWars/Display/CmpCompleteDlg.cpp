#include "CmpCompleteDlg.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "Campaign.h"
#include "CmpnScreen.h"
#include "SSWGameInstance.h"

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
    }

    if (const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        if (GI->GetActiveCampaignUIBundle().LoadTop)
        {
            BgTop->SetBrushFromTexture(GI->GetActiveCampaignUIBundle().LoadTop);
            return;
        }
    }

    BgTop->SetColorAndOpacity(FLinearColor::Black);
}

void UCmpCompleteDlg::BuildCenterBanner()
{
    TitleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleImage"));

    UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(TitleImage);
    if (CenterSlot)
    {
        CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CenterSlot->SetSize(FVector2D(1024.f, 512.f));
    }
}

void UCmpCompleteDlg::BuildBottomPanel()
{
    BgBottom = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BgBottom"));

    UCanvasPanelSlot* BottomSlot = RootCanvas->AddChildToCanvas(BgBottom);
    if (BottomSlot)
    {
        BottomSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
        BottomSlot->SetAlignment(FVector2D(0.f, 1.f));
        BottomSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 140.f));
    }

    if (const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        if (GI->GetActiveCampaignUIBundle().LoadBottom)
        {
            BgBottom->SetBrushFromTexture(GI->GetActiveCampaignUIBundle().LoadBottom);
        }
    }

    InfoLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoLabel"));
    InfoLabel->SetText(FText::FromString(TEXT("CAMPAIGN COMPLETE")));
    InfoLabel->SetJustification(ETextJustify::Center);

    RootCanvas->AddChildToCanvas(InfoLabel);

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
    RootCanvas->AddChildToCanvas(CloseButton);

    CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
    CloseButtonText->SetText(FText::FromString(TEXT("CLOSE")));

    CloseButton->AddChild(CloseButtonText);
}

void UCmpCompleteDlg::ShowCompleteDlg()
{
    ShowTime = 0.0f;

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
    SetDialogInputEnabled(true);

    if (const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance()))
    {
        if (GI->GetActiveCampaignUIBundle().CampaignComplete)
        {
            TitleImage->SetBrushFromTexture(GI->GetActiveCampaignUIBundle().CampaignComplete);
        }
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