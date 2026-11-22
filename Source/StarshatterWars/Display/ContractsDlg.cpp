#include "ContractsDlg.h"
#include "CommerceCharges.h"
#include "ChangeDrivenPanel.h"
#include "RecordDetails.h"

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

    FRecordDetails DescribeContract(const Sim::FActiveHaul& Haul)
    {
        const auto& Terms=Haul.Terms;FRecordDetails D;D.Icon=EShipSystemIcon::Contracts;
        D.Title=FText::FromString(Terms.Title);D.Description=FText::FromString(Terms.Description);
        D.Status=Haul.bDelivering?INVTEXT("DELIVERING"):INVTEXT("LOADING");
        D.Subtitle=StaticEnum<ESSWContractType>()->GetDisplayNameTextByValue(static_cast<int64>(Terms.Type));
        D.Fact(INVTEXT("PICKUP"),FText::FromName(Terms.PickupStationId)).Fact(INVTEXT("DESTINATION"),FText::FromName(Terms.DestinationStationId));
        D.Fact(INVTEXT("ACCESS"),Terms.bPrivate?INVTEXT("PRIVATE"):INVTEXT("PUBLIC"));
        D.Fact(INVTEXT("ROUTE"),Haul.Jumps>=0?FText::Format(INVTEXT("{0} JUMPS"),FText::AsNumber(Haul.Jumps)):INVTEXT("UNAVAILABLE"));
        if(!Terms.AssignedToId.IsNone())D.Fact(INVTEXT("ASSIGNED TO"),FText::FromName(Terms.AssignedToId));
        if(!Terms.PickupDivisionId.IsNone())D.Fact(INVTEXT("DIVISION"),FText::FromName(Terms.PickupDivisionId));
        const auto Credits=[&D](const FText& Label,int64 Amount){if(Amount>0)D.Fact(Label,FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Amount)));};
        Credits(INVTEXT("PRICE"),Terms.PriceCredits);Credits(INVTEXT("REWARD"),Terms.RewardCredits);Credits(INVTEXT("COLLATERAL"),Terms.CollateralCredits);
        if(Terms.PriceCredits>0){const auto Price=CommerceCharges::Quote(Terms.PriceCredits,ECommercePurpose::MarketSale);D.Fact(INVTEXT("INCLUDED GST"),FText::AsNumber(Price.Tax));D.Fact(INVTEXT("SELLER NET"),FText::AsNumber(Price.Base));}
        if(Terms.OfferExpiryMinutes>0)D.Fact(INVTEXT("OFFER WINDOW"),FText::Format(INVTEXT("{0} MIN"),FText::AsNumber(Terms.OfferExpiryMinutes)));
        if(Terms.CompletionDays>0)D.Fact(INVTEXT("COMPLETION WINDOW"),FText::Format(INVTEXT("{0} DAYS"),FText::AsNumber(Terms.CompletionDays)));
        D.Section(INVTEXT("OFFERED CARGO"),FText::FromString(DescribeItems(Terms.OfferedItems)));
        D.Section(INVTEXT("REQUESTED ITEMS"),FText::FromString(DescribeItems(Terms.RequestedItems)));
        if(Terms.bOnBehalfOfCorporation)D.Fact(INVTEXT("ISSUER"),INVTEXT("CORPORATION"));
        if(Terms.bMultiContract)D.Fact(INVTEXT("BUNDLE"),INVTEXT("MULTIPLE CONTRACTS"));return D;
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
    const auto AbandonArmed = MakeShared<bool>(false);
    const auto Feedback = MakeShared<FText>();
    const auto BidInput = SNew(SEditableTextBox).HintText(INVTEXT("Bid in credits"));
    auto Rows = SNew(SScrollBox);
    const auto Refresh = [Rows, Query, Tab, Selected, SelectedId, Feedback, BidInput, AbandonArmed]()
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
                }), FOnClicked::CreateLambda([SelectedId, Feedback, BidInput, AbandonArmed, Id = Entry.Key]()
                {
                    *AbandonArmed = false;
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
    const auto Execute = [SelectedId, Feedback, Refresh, Tab, AbandonArmed](Action Task, int64 Credits)
    {
        *AbandonArmed = false;
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
    Actions->AddSlot().AutoWidth().Padding(0,0,6,0)
    [SNew(SButton)
        .Text_Lambda([AbandonArmed](){return *AbandonArmed ? INVTEXT("CONFIRM ABANDON") : INVTEXT("ABANDON");})
        .ToolTipText(INVTEXT("Abandon your active courier contract. The sealed package is removed and collateral is forfeited. Click twice to confirm."))
        .Visibility_Lambda([GetSelectedOffer](){const auto* Offer=GetSelectedOffer();return Offer && Offer->Terms.Type==ESSWContractType::Courier &&
            Offer->State==EContractState::InProgress && Offer->Acceptor==Sim::TrainingContractPilot ? EVisibility::Visible : EVisibility::Collapsed;})
        .OnClicked_Lambda([AbandonArmed,GetSelectedOffer,Feedback,Execute]()
        {
            const auto* Offer=GetSelectedOffer();if (!Offer)return FReply::Handled();
            if (*AbandonArmed)return Execute(Action::Abandon,0);
            *AbandonArmed=true;
            *Feedback=FText::Format(INVTEXT("Abandoning removes the package and forfeits {0} cr collateral. Click CONFIRM ABANDON to proceed."),FText::AsNumber(Offer->Terms.CollateralCredits));
            return FReply::Handled();
        })];
    AddAction(INVTEXT("TARGET FREIGHTER"), [GetLocalFreighter]() { return GetLocalFreighter() != nullptr; },
        FOnClicked::CreateLambda([GetLocalFreighter]()
        {
            if (Ship* Freighter = GetLocalFreighter())
                Sim::GetSim()->GetPlayerShip()->SetTarget(Freighter);
            return FReply::Handled();
        }));

    return SNew(SChangeDrivenPanel).OnChanged_Lambda([Refresh](){Refresh();})[SNew(SVerticalBox)
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
                            SNew(SRecordDetails).EventDriven(true).SelectionRevision_Lambda([Selected,GetSelectedOffer,Tab](){return uint32(*Tab)^((*Selected).IsSet()?GetTypeHash(Selected->GetValue().Id):PointerHash(GetSelectedOffer()));}).Record_Lambda([Selected, GetSelectedOffer, Tab]()
                            {
                                if(*Tab==ETab::Hauls && Selected->IsSet())return DescribeContract(Selected->GetValue());
                                const auto* Offer=GetSelectedOffer();
                                if(!Offer){FRecordDetails Empty;Empty.Icon=EShipSystemIcon::Contracts;Empty.Title=INVTEXT("SELECT A CONTRACT");Empty.Description=INVTEXT("Review the route, cargo and financial terms before accepting or bidding.");return Empty;}
                                Sim::FActiveHaul View;View.Terms=Offer->Terms;View.Jumps=-1;FRecordDetails D=DescribeContract(View);
                                const TCHAR* States[]={TEXT("Draft"),TEXT("Open"),TEXT("In progress"),TEXT("Awaiting claims"),TEXT("Completed"),TEXT("Failed"),TEXT("Expired")};
                                D.Status=FText::FromString(States[FMath::Clamp(int32(Offer->State),0,int32(UE_ARRAY_COUNT(States))-1)]);
                                if(Offer->Terms.Type!=ESSWContractType::Courier){
                                    D.Fact(INVTEXT("OPENING BID"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Offer->Terms.PriceCredits)));
                                    D.Fact(INVTEXT("HIGH BID"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Offer->HighBid)));
                                    D.Fact(INVTEXT("BUYOUT"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Offer->Buyout)));
                                }
                                if(Offer->Terms.Type==ESSWContractType::Auction && Offer->State==EContractState::Open){
                                    const int32 Seconds=FMath::Max(0,FMath::CeilToInt((Offer->ExpiresAt-FDateTime::UtcNow()).GetTotalSeconds()));
                                    D.Fact(INVTEXT("CLOSES IN"),FText::FromString(FString::Printf(TEXT("%d:%02d"),Seconds/60,Seconds%60)));
                                }return D;
                            })
                        ]
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 8, 0, 0)
                [Actions]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                [SNew(STextBlock).AutoWrapText(true).Text_Lambda([Feedback]() { return *Feedback; })]
            )
        ]];
}
