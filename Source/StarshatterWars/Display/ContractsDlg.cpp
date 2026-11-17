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
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    int64 ParseBid(const FString& Input)
    {
        int64 Amount = 0;
        for (const TCHAR Character : Input)
        {
            if (Character < TEXT('0') || Character > TEXT('9'))
                return 0;
            const int64 Digit = Character - TEXT('0');
            if (Amount > (MAX_int64 - Digit) / 10)
                return 0;
            Amount = Amount * 10 + Digit;
        }
        return Amount;
    }

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
    using Action = Sim::ETrainingContractAction;
    enum class ETab { Offers, Personal, Hauls };
    Sim* InitialSim = Sim::GetSim();
    const bool bTraining = InitialSim && !InitialSim->GetTrainingContracts().IsEmpty();
    const auto Tab = MakeShared<ETab>(bTraining ? ETab::Offers : ETab::Hauls);
    const auto Query = MakeShared<FString>();
    const auto Selected = MakeShared<TOptional<Sim::FActiveHaul>>();
    const auto SelectedId = MakeShared<FName>();
    const auto Feedback = MakeShared<FText>();
    const auto BidInput = SNew(SEditableTextBox).HintText(INVTEXT("Bid in credits"));
    auto Rows = SNew(SScrollBox);
    const auto Refresh = [Rows, Query, Tab, Selected, SelectedId, Feedback, BidInput]()
    {
        Rows->ClearChildren();
        Sim* Simulation = Sim::GetSim();
        const auto AddRow = [&Rows, &Query](const FString& Text, TAttribute<FSlateColor> Color, FOnClicked Click)
        {
            if (Query->IsEmpty() || Text.Contains(*Query))
                Rows->AddSlot().Padding(0, 0, 0, 4)
                [
                    SNew(SButton).HAlign(HAlign_Left).ButtonColorAndOpacity(Color).OnClicked(Click)
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Text).ToUpper())]
                ];
        };
        if (Simulation && *Tab != ETab::Hauls)
        {
            for (const auto& Entry : Simulation->GetTrainingContracts())
            {
                const auto& Contract = Entry.Value;
                const bool bMine = Contract.Issuer == Sim::TrainingContractPilot ||
                    Contract.Acceptor == Sim::TrainingContractPilot || Contract.HighBidder == Sim::TrainingContractPilot;
                if (*Tab == ETab::Personal ? !bMine : Contract.State != EContractState::Open)
                    continue;
                const FString Row = FString::Printf(TEXT("%s  |  %s"), *Contract.Terms.Title, *Contract.Terms.PickupStationId.ToString());
                AddRow(Row, TAttribute<FSlateColor>::CreateLambda([SelectedId, Id = Entry.Key]()
                {
                    return FSlateColor(*SelectedId == Id ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG);
                }), FOnClicked::CreateLambda([SelectedId, Feedback, BidInput, Id = Entry.Key]()
                {
                    *SelectedId = Id;
                    *Feedback = FText::GetEmpty();
                    BidInput->SetText(FText::GetEmpty());
                    return FReply::Handled();
                }));
            }
        }
        else if (Simulation)
        {
            const FGuid Previous = Selected->IsSet() ? Selected->GetValue().Id : FGuid();
            Selected->Reset();
            for (const auto& Haul : Simulation->GetActiveHauls())
            {
                const auto& Terms = Haul.Terms;
                const FString Row = FString::Printf(TEXT("%s  |  %s → %s  |  %s"), *Terms.Title,
                    *Terms.PickupStationId.ToString(), *Terms.DestinationStationId.ToString(), *DescribeItems(Terms.OfferedItems));
                if (Haul.Id == Previous) *Selected = Haul;
                AddRow(Row, TAttribute<FSlateColor>::CreateLambda([Selected, Id = Haul.Id]()
                {
                    return FSlateColor(Selected->IsSet() && Selected->GetValue().Id == Id
                        ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG);
                }), FOnClicked::CreateLambda([Selected, Haul]() { *Selected = Haul; return FReply::Handled(); }));
            }
        }
        if (Rows->GetChildren()->Num() == 0)
            Rows->AddSlot()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(INVTEXT("No matching contracts"))];
    };
    Refresh();

    const auto GetSelectedOffer = [Tab, SelectedId]() -> const FContractTransaction*
    {
        Sim* Simulation = Sim::GetSim();
        return *Tab != ETab::Hauls && Simulation && !SelectedId->IsNone()
            ? Simulation->GetTrainingContracts().Find(*SelectedId) : nullptr;
    };
    const auto Execute = [SelectedId, Feedback, Refresh, Tab](Action Task, int64 Credits)
    {
        if (Sim* Simulation = Sim::GetSim())
        {
            *Feedback = Simulation->ExecuteTrainingContract(*SelectedId, Task, Credits);
            if (Task == Action::Accept)
                if (const auto* Offer = Simulation->GetTrainingContracts().Find(*SelectedId);
                    Offer && Offer->Acceptor == Sim::TrainingContractPilot)
                    *Tab = ETab::Personal;
            Refresh();
        }
        return FReply::Handled();
    };

    auto Tabs = SNew(SHorizontalBox);
    const auto AddTab = [&Tabs, Tab, Refresh, SelectedId, Feedback](const FText& Label, ETab Value)
    {
        Tabs->AddSlot().AutoWidth().Padding(0, 0, 16, 0)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([Tab, Value]() { return *Tab == Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([Tab, Value, Refresh, SelectedId, Feedback](ECheckBoxState)
            {
                *Tab = Value;
                *SelectedId = NAME_None;
                *Feedback = FText::GetEmpty();
                Refresh();
            })
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(Label)]
        ];
    };
    if (bTraining)
    {
        AddTab(INVTEXT("TRAINING OFFERS"), ETab::Offers);
        AddTab(INVTEXT("MY CONTRACTS"), ETab::Personal);
    }
    AddTab(INVTEXT("ACTIVE HAULS"), ETab::Hauls);
    Tabs->AddSlot().AutoWidth()
    [
        SNew(SButton).Text(INVTEXT("REFRESH"))
        .OnClicked_Lambda([Refresh]() { Refresh(); return FReply::Handled(); })
    ];

    const auto GetLocalFreighter = [Selected, Tab]() -> Ship*
    {
        Sim* Simulation = Sim::GetSim();
        Ship* Player = Simulation ? Simulation->GetPlayerShip() : nullptr;
        if (*Tab != ETab::Hauls || !Player || !Selected->IsSet() || !Player->GetRegion())
            return nullptr;
        Ship* Freighter = Simulation->FindShip(TCHAR_TO_UTF8(*Selected->GetValue().Terms.AssignedToId.ToString()));
        return Freighter && Freighter->GetRegion() == Player->GetRegion() &&
            !Freighter->IsDead() && !Freighter->IsDying() ? Freighter : nullptr;
    };

    const auto CanBid = [GetSelectedOffer]()
    {
        const auto* Offer = GetSelectedOffer();
        return Offer && Offer->Terms.Type == ESSWContractType::Auction && Offer->State == EContractState::Open;
    };
    auto Actions = SNew(SHorizontalBox);
    const auto AddAction = [&Actions](FText Label, TFunction<bool()> Available, FOnClicked Click)
    {
        Actions->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
        [
            SNew(SButton).Text(Label).OnClicked(Click)
            .Visibility_Lambda([Available]() { return Available() ? EVisibility::Visible : EVisibility::Collapsed; })
        ];
    };
    AddAction(INVTEXT("ACCEPT"), [GetSelectedOffer]()
    {
        const auto* Offer = GetSelectedOffer();
        return Offer && Offer->State == EContractState::Open && Offer->Terms.Type == ESSWContractType::Courier;
    }, FOnClicked::CreateLambda([Execute]() { return Execute(Action::Accept, 0); }));
    Actions->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
    [
        SNew(SBox).WidthOverride(130)
        .Visibility_Lambda([CanBid]() { return CanBid() ? EVisibility::Visible : EVisibility::Collapsed; })
        [BidInput]
    ];
    AddAction(INVTEXT("PLACE BID"), CanBid, FOnClicked::CreateLambda([Execute, BidInput, Feedback]()
    {
        const FString Input = BidInput->GetText().ToString().TrimStartAndEnd();
        const int64 Credits = ParseBid(Input);
        if (Credits <= 0)
        {
            *Feedback = INVTEXT("Enter a positive whole-credit bid.");
            return FReply::Handled();
        }
        return Execute(Action::Bid, Credits);
    }));
    AddAction(INVTEXT("BUYOUT"), [CanBid, GetSelectedOffer]() { return CanBid() && GetSelectedOffer()->Buyout > 0; },
        FOnClicked::CreateLambda([Execute, GetSelectedOffer]()
        {
            if (const auto* Offer = GetSelectedOffer()) return Execute(Action::Bid, Offer->Buyout);
            return FReply::Handled();
        }));
    AddAction(INVTEXT("DELIVER"), [GetSelectedOffer]()
    {
        const auto* Offer = GetSelectedOffer();
        return Offer && Offer->State == EContractState::InProgress && Offer->Acceptor == Sim::TrainingContractPilot;
    }, FOnClicked::CreateLambda([Execute]() { return Execute(Action::Deliver, 0); }));
    const auto FindDestination = [GetSelectedOffer]() -> Ship*
    {
        const auto* Offer = GetSelectedOffer();
        Sim* CurrentSim = Sim::GetSim();
        Ship* Player = CurrentSim ? CurrentSim->GetPlayerShip() : nullptr;
        if (!Offer || Offer->State != EContractState::InProgress || !Player || !Player->GetRegion())
            return nullptr;
        Ship* Station = CurrentSim->FindShip(TCHAR_TO_UTF8(*Offer->Terms.DestinationStationId.ToString()));
        return Station && Station->GetRegion() == Player->GetRegion() && !Station->IsDead() && !Station->IsDying()
            ? Station : nullptr;
    };
    AddAction(INVTEXT("TARGET DESTINATION"), [FindDestination]() { return FindDestination() != nullptr; },
        FOnClicked::CreateLambda([FindDestination]()
        {
            if (Ship* Station = FindDestination()) Sim::GetSim()->GetPlayerShip()->SetTarget(Station);
            return FReply::Handled();
        }));
    AddAction(INVTEXT("AUTO TRACK DESTINATION"), [FindDestination]() { return FindDestination() != nullptr; },
        FOnClicked::CreateLambda([FindDestination, Feedback]()
        {
            if (Ship* Station = FindDestination())
            {
                Ship* Player = Sim::GetSim()->GetPlayerShip();
                Player->SetTarget(Station);
                const bool bTracking = Player->SetFixedAutoTrackPoint(Station->GetLocation(), UTF8_TO_TCHAR(Station->GetName()));
                *Feedback = bTracking ? INVTEXT("Auto Track engaged. Close Contracts to return to flight.")
                                      : INVTEXT("Auto Track needs an airborne fighter with navigation powered.");
            }
            return FReply::Handled();
        }));
    AddAction(INVTEXT("CLAIM"), [GetSelectedOffer]()
    {
        const auto* Offer = GetSelectedOffer();
        return Offer && Offer->State == EContractState::AwaitingClaims &&
            ((Offer->Issuer == Sim::TrainingContractPilot && !Offer->bSellerClaimed) ||
             (Offer->HighBidder == Sim::TrainingContractPilot && !Offer->bBuyerClaimed));
    }, FOnClicked::CreateLambda([Execute]() { return Execute(Action::Claim, 0); }));
    AddAction(INVTEXT("TARGET FREIGHTER"), [GetLocalFreighter]() { return GetLocalFreighter() != nullptr; },
        FOnClicked::CreateLambda([GetLocalFreighter]()
        {
            if (Ship* Freighter = GetLocalFreighter())
                Sim::GetSim()->GetPlayerShip()->SetTarget(Freighter);
            return FReply::Handled();
        }));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(12, 10)
        [Tabs]
        + SVerticalBox::Slot().AutoHeight().Padding(12, 0, 12, 8)
        [
            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
            .Visibility(bTraining ? EVisibility::Visible : EVisibility::Collapsed)
            .Text_Lambda([]()
            {
                const Sim* Simulation = Sim::GetSim();
                return FText::Format(INVTEXT("Training balance  {0} cr"), FText::AsNumber(Simulation ? Simulation->GetTrainingContractBalance() : int64(0)));
            })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(12, 0, 12, 10)
        [
            SNew(SSearchBox)
            .HintText(INVTEXT("Search cargo, freighter or station"))
            .OnTextChanged_Lambda([Query, Refresh](const FText& Text) { *Query = Text.ToString().TrimStartAndEnd(); Refresh(); })
        ]
        + SVerticalBox::Slot().FillHeight(1.0f).Padding(12, 0)
        [MissionUIStyle::MakeDataPane(Rows)]
        + SVerticalBox::Slot().AutoHeight().Padding(12, 8)
        [
            MissionUIStyle::MakeDataPane(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBox).MaxDesiredHeight(270.f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
                            .Text_Lambda([Selected, GetSelectedOffer, Tab]()
                            {
                                if (*Tab == ETab::Hauls && Selected->IsSet()) return DescribeContract(Selected->GetValue());
                                const auto* Offer = GetSelectedOffer();
                                if (!Offer) return INVTEXT("SELECT A CONTRACT");
                                const auto& Terms = Offer->Terms;
                                FString Body = FString::Printf(TEXT("%s\n%s\nPickup  %s\n%s"), *Terms.Title,
                                    *Terms.Description, *Terms.PickupStationId.ToString(), *DescribeItems(Terms.OfferedItems));
                                const TCHAR* States[] = {TEXT("Draft"), TEXT("Open"), TEXT("In progress"),
                                    TEXT("Awaiting claims"), TEXT("Completed"), TEXT("Failed"), TEXT("Expired")};
                                Body += FString::Printf(TEXT("\nStatus  %s"), States[static_cast<uint8>(Offer->State)]);
                                if (Terms.Type == ESSWContractType::Courier)
                                    Body += FString::Printf(TEXT("\nDestination  %s\nReward  %lld cr  |  Collateral  %lld cr"),
                                        *Terms.DestinationStationId.ToString(), Terms.RewardCredits, Terms.CollateralCredits);
                                else
                                    Body += FString::Printf(TEXT("\nOpening bid  %lld cr  |  High bid  %lld cr  |  Buyout  %lld cr"),
                                        Terms.PriceCredits, Offer->HighBid, Offer->Buyout);
                                if (Terms.Type == ESSWContractType::Auction && Offer->State == EContractState::Open)
                                {
                                    const auto Remaining = Offer->ExpiresAt - FDateTime::UtcNow();
                                    const int32 Seconds = FMath::Max(0, FMath::CeilToInt(Remaining.GetTotalSeconds()));
                                    Body += FString::Printf(TEXT("\nCloses in  %d:%02d"), Seconds / 60, Seconds % 60);
                                }
                                return FText::FromString(Body);
                            })
                        ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 8, 0, 0)
                [Actions]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                [SNew(STextBlock).AutoWrapText(true).Text_Lambda([Feedback]() { return *Feedback; })]
            )
        ];
}
