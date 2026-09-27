#include "CmdOrdersDlg.h"

#include "CmdDlg.h"
#include "GameStructs.h"
#include "Components/TextBlock.h"

UCmdOrdersDlg::UCmdOrdersDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmdOrdersDlg::NativeConstruct()
{
    Super::NativeConstruct();
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

    TArray<FString> Orders = CampaignData.Orders;
    Orders.SetNum(4);

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::FromString(CampaignData.Name));
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