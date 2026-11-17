#include "ContractsDlg.h"

#include "MissionUIStyle.h"
#include "Sim.h"
#include "Ship.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    FString DescribeItems(const TArray<FSSWContractItem>& Items)
    {
        TArray<FString> Lines;
        for (const FSSWContractItem& Item : Items)
        {
            Lines.Add(FString::Printf(TEXT("%s  •  %s %s"), *Item.ItemId.ToString(),
                *FText::AsNumber(Item.Quantity).ToString(), *Item.Unit.ToString()));
        }
        return FString::Join(Lines, TEXT("\n"));
    }

    FText DescribeContract(const Sim::FActiveHaul& Haul)
    {
        const FSSWContractTerms& Terms = Haul.Terms;
        TArray<FString> Lines;
        const auto Add = [&Lines](const TCHAR* Label, const FString& Value)
        {
            if (!Value.IsEmpty())
                Lines.Add(FString::Printf(TEXT("%s  %s"), Label, *Value));
        };
        const auto AddStationField = [&Add](const TCHAR* Label, FName Value)
        {
            if (!Value.IsNone())
                Add(Label, Value.ToString());
        };
        const auto AddCredits = [&Add](const TCHAR* Label, int64 Credits)
        {
            if (Credits > 0)
                Add(Label, FText::AsNumber(Credits).ToString() + TEXT(" cr"));
        };
        Lines.Add(Terms.Title);
        if (!Terms.Description.IsEmpty()) Lines.Add(Terms.Description);
        Add(TEXT("TYPE"), StaticEnum<ESSWContractType>()->GetDisplayNameTextByValue(static_cast<int64>(Terms.Type)).ToString());
        Add(TEXT("STATUS"), Haul.bDelivering ? TEXT("Delivering") : TEXT("Loading"));
        Add(TEXT("ACCESS"), Terms.bPrivate ? TEXT("Private") : TEXT("Public"));
        AddStationField(TEXT("ASSIGNED TO"), Terms.AssignedToId);
        AddStationField(TEXT("PICKUP"), Terms.PickupStationId);
        AddStationField(TEXT("DIVISION"), Terms.PickupDivisionId);
        AddStationField(TEXT("DESTINATION"), Terms.DestinationStationId);
        Add(TEXT("JUMPS"), Haul.Jumps >= 0 ? FString::FromInt(Haul.Jumps) : TEXT("Unavailable"));
        Add(TEXT("OFFERED CARGO"), DescribeItems(Terms.OfferedItems));
        Add(TEXT("REQUESTED ITEMS"), DescribeItems(Terms.RequestedItems));
        AddCredits(TEXT("PRICE"), Terms.PriceCredits);
        AddCredits(TEXT("REWARD"), Terms.RewardCredits);
        AddCredits(TEXT("COLLATERAL"), Terms.CollateralCredits);
        if (Terms.OfferExpiryMinutes > 0)
            Add(TEXT("OFFER WINDOW"), FString::Printf(TEXT("%d min"), Terms.OfferExpiryMinutes));
        if (Terms.CompletionDays > 0)
            Add(TEXT("COMPLETION WINDOW"), FString::Printf(TEXT("%d d"), Terms.CompletionDays));
        if (Terms.bOnBehalfOfCorporation) Lines.Add(TEXT("Issued on behalf of a corporation"));
        if (Terms.bMultiContract) Lines.Add(TEXT("Multiple contracts"));
        return FText::FromString(FString::Join(Lines, TEXT("\n")));
    }
}

TSharedRef<SWidget> UContractsDlg::RebuildWidget()
{
    const auto bPersonal = MakeShared<bool>(false);
    const auto Query = MakeShared<FString>();
    const auto Selected = MakeShared<TOptional<Sim::FActiveHaul>>();
    auto Rows = SNew(SScrollBox);
    const auto Refresh = [Rows, Query, bPersonal, Selected]()
    {
        Rows->ClearChildren();
        const FGuid PreviousSelection = Selected->IsSet() ? Selected->GetValue().Id : FGuid();
        Selected->Reset();
        Sim* Simulation = Sim::GetSim();
        const auto Hauls = Simulation && !*bPersonal ? Simulation->GetActiveHauls() : TArray<Sim::FActiveHaul>{};
        for (const auto& Haul : Hauls)
        {
            const FSSWContractTerms& Terms = Haul.Terms;
            const FString Cargo = DescribeItems(Terms.OfferedItems);
            const FString Row = FString::Printf(TEXT("%s   |   %s → %s   |   %s"),
                *Terms.Title, *Terms.PickupStationId.ToString(), *Terms.DestinationStationId.ToString(), *Cargo);
            if (!Query->IsEmpty() && !Row.Contains(*Query) && !Terms.AssignedToId.ToString().Contains(*Query))
                continue;

            if (Haul.Id == PreviousSelection) *Selected = Haul;
            Rows->AddSlot().Padding(0, 0, 0, 4)
            [
                SNew(SButton)
                .HAlign(HAlign_Left)
                .OnClicked_Lambda([Selected, Haul]() { *Selected = Haul; return FReply::Handled(); })
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(FText::FromString(Row))]
            ];
        }
        if (Rows->GetChildren()->Num() == 0)
            Rows->AddSlot()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
                .Text(*bPersonal ? INVTEXT("NO PERSONAL CONTRACTS") : INVTEXT("NO MATCHING ACTIVE HAULS"))];
    };
    Refresh();

    auto Tabs = SNew(SHorizontalBox);
    const auto AddTab = [&Tabs, bPersonal, Refresh](const FText& Label, bool bMyContracts)
    {
        Tabs->AddSlot().AutoWidth().Padding(0, 0, 16, 0)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([bPersonal, bMyContracts]()
            {
                return *bPersonal == bMyContracts ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([bPersonal, bMyContracts, Refresh](ECheckBoxState)
            {
                *bPersonal = bMyContracts;
                Refresh();
            })
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(Label)]
        ];
    };
    AddTab(INVTEXT("ACTIVE HAULS"), false);
    AddTab(INVTEXT("MY CONTRACTS"), true);
    Tabs->AddSlot().AutoWidth()
    [
        SNew(SButton).Text(INVTEXT("REFRESH"))
        .OnClicked_Lambda([Refresh]() { Refresh(); return FReply::Handled(); })
    ];

    const auto GetLocalFreighter = [Selected]() -> Ship*
    {
        Sim* Simulation = Sim::GetSim();
        Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
        if (!Player || !Selected->IsSet() || !Player->GetRegion())
            return nullptr;
        Ship* Freighter = Simulation->FindShip(TCHAR_TO_UTF8(*Selected->GetValue().Terms.AssignedToId.ToString()));
        return Freighter && Freighter->GetRegion() == Player->GetRegion() &&
            !Freighter->IsDead() && !Freighter->IsDying() ? Freighter : nullptr;
    };

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(12, 10)
        [Tabs]
        + SVerticalBox::Slot().AutoHeight().Padding(12, 0, 12, 10)
        [
            SNew(SSearchBox)
            .HintText(INVTEXT("Search cargo, freighter or station"))
            .OnTextChanged_Lambda([Query, Refresh](const FText& Text) { *Query = Text.ToString().TrimStartAndEnd(); Refresh(); })
        ]
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(12, 0)
        [Rows]
        + SVerticalBox::Slot().AutoHeight().Padding(12, 8)
        [
            SNew(SBorder).Padding(12).BorderBackgroundColor(FLinearColor(0.15f, 0.18f, 0.21f, 1.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).MaxDesiredHeight(270.f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
                            .Text_Lambda([Selected]()
                            {
                                return Selected->IsSet() ? DescribeContract(Selected->GetValue()) : INVTEXT("SELECT A CONTRACT");
                            })
                        ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 8, 0, 0)
                [
                    SNew(SButton).Text(INVTEXT("TARGET FREIGHTER"))
                    .IsEnabled_Lambda([GetLocalFreighter]() { return GetLocalFreighter() != nullptr; })
                    .OnClicked_Lambda([GetLocalFreighter]()
                    {
                        if (Ship* Freighter = GetLocalFreighter())
                            Sim::GetSim()->GetPlayerShip()->SetTarget(Freighter);
                        return FReply::Handled();
                    })
                ]
            ]
        ];
}
