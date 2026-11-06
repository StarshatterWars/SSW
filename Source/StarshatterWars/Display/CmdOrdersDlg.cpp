#include "CmdOrdersDlg.h"

#include "CmdDlg.h"
#include "GameStructs.h"
#include "MissionUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

UCmdOrdersDlg::UCmdOrdersDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdOrdersDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

TSharedRef<SWidget> UCmdOrdersDlg::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget && !CampaignArtFrame)
    {
        UWidget* OrdersContent = WidgetTree->RootWidget;
        UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>();
        WidgetTree->RootWidget = Layout;
        Layout->AddChildToHorizontalBox(OrdersContent)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        CampaignArtFrame = WidgetTree->ConstructWidget<USizeBox>();
        CampaignArtFrame->SetWidthOverride(340.f);
        CampaignArtFrame->SetVisibility(ESlateVisibility::Collapsed);
        Layout->AddChildToHorizontalBox(CampaignArtFrame)->SetPadding(FMargin(16.f, 16.f, 24.f, 16.f));
        UScaleBox* ArtScale = WidgetTree->ConstructWidget<UScaleBox>();
        ArtScale->SetStretch(EStretch::ScaleToFit);
        CampaignArtFrame->SetContent(ArtScale);
        CampaignArt = WidgetTree->ConstructWidget<UImage>();
        ArtScale->SetContent(CampaignArt);
    }
    return Super::RebuildWidget();
}

void UCmdOrdersDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
    ParentCmdDlg = InParentCmdDlg;
}

void UCmdOrdersDlg::ShowOrdersDlg()
{
    SetCampaignOrders();
    SetVisibility(ESlateVisibility::Visible);
}

void UCmdOrdersDlg::SetCampaignOrders()
{
    if (!ParentCmdDlg || !ParentCmdDlg->HasCurrentCampaign())
    {
        if (CampaignArtFrame) CampaignArtFrame->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Warning, TEXT("[CmdOrdersDlg] SetCampaignOrders: No parent campaign data"));

        if (CampaignNameText)
        {
            CampaignNameText->SetText(FText::GetEmpty());
        }

        if (DescriptionText)
        {
            DescriptionText->SetText(FText::GetEmpty());
        }

        if (SituationText)
        {
            SituationText->SetText(FText::GetEmpty());
        }

        if (Orders1Text)
        {
            Orders1Text->SetText(FText::GetEmpty());
        }

        if (Orders2Text)
        {
            Orders2Text->SetText(FText::GetEmpty());
        }

        if (Orders3Text)
        {
            Orders3Text->SetText(FText::GetEmpty());
        }

        if (Orders4Text)
        {
            Orders4Text->SetText(FText::GetEmpty());
        }

        if (LocationSystemText)
        {
            LocationSystemText->SetText(FText::GetEmpty());
        }

        return;
    }

    const FS_Campaign& CampaignData = ParentCmdDlg->GetCurrentCampaignData();
    if (CampaignArt)
    {
        UTexture2D* Texture = CampaignData.Index >= 2 ? MissionUIStyle::LoadCampaignArt(CampaignData.Index) : nullptr;
        CampaignArt->SetBrushFromTexture(Texture, true);
        CampaignArtFrame->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    TArray<FString> Orders = CampaignData.Orders;
    Orders.SetNum(4);

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::GetEmpty());
        CampaignNameText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(CampaignData.Description));
    }

    if (SituationText)
    {
        SituationText->SetText(FText::FromString(CampaignData.Situation));
    }

    if (Orders1Text)
    {
        Orders1Text->SetText(FText::FromString(Orders[0]));
    }

    if (Orders2Text)
    {
        Orders2Text->SetText(FText::FromString(Orders[1]));
    }

    if (Orders3Text)
    {
        Orders3Text->SetText(FText::FromString(Orders[2]));
    }

    if (Orders4Text)
    {
        Orders4Text->SetText(FText::FromString(Orders[3]));
    }

    if (LocationSystemText)
    {
        const FString LocationSystem = CampaignData.System + TEXT("/") + CampaignData.Region;
        LocationSystemText->SetText(FText::FromString(LocationSystem));
    }
}
