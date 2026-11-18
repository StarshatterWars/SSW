#include "MarketplaceDlg.h"
#include "ShipSystemIcon.h"
#include "StationServices.h"
#include "EconomyRuntime.h"
#include "StarshatterPlayerSubsystem.h"
#include "Ship.h"
#include "Misc/DateTime.h"
#include "Widgets/SOverlay.h"

#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameStructs.h"
#include "Mission.h"
#include "MissionUIStyle.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "StarSystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "Widgets/Input/SSearchBox.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
    struct FSystemMarket
    {
        FString Name;
        double DemandScale = 1;
        bool bIllicitTrade = false;
        TSet<FName> Production;
        TSet<FName> ExclusiveSupply;
    };

    TArray<FSystemMarket> LoadSystemMarkets()
    {
        FString Json;
        TArray<TSharedPtr<FJsonValue>> Entries;
        TArray<FSystemMarket> Markets;
        if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("GameData/Cargo/economy.json"))) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Entries))
            return Markets;
        for (const auto& Entry : Entries)
        {
            const TSharedPtr<FJsonObject>* Object;
            if (!Entry->TryGetObject(Object)) continue;
            FSystemMarket Market;
            if (!(*Object)->TryGetStringField(TEXT("Name"), Market.Name) || Market.Name.IsEmpty()) continue;
            (*Object)->TryGetNumberField(TEXT("DemandScale"), Market.DemandScale);
            (*Object)->TryGetBoolField(TEXT("bIllicitTrade"), Market.bIllicitTrade);
            const TArray<TSharedPtr<FJsonValue>>* Products;
            if ((*Object)->TryGetArrayField(TEXT("Production"), Products))
                for (const auto& Product : *Products)
                    Market.Production.Add(DataTableUtils::MakeValidName(Product->AsString()));
            const TArray<TSharedPtr<FJsonValue>>* ExclusiveGoods;
            if ((*Object)->TryGetArrayField(TEXT("ExclusiveSupply"), ExclusiveGoods))
                for (const auto& Product : *ExclusiveGoods)
                    Market.ExclusiveSupply.Add(DataTableUtils::MakeValidName(Product->AsString()));
            Markets.Add(MoveTemp(Market));
        }
        return Markets;
    }

    TMap<FName, int32> GetJumpDistances(const UStarshatterEnvironmentSubsystem* Environment, const FString& Origin)
    {
        TMap<FName, int32> Distances;
        if (Origin.IsEmpty()) return Distances;
        Distances.Add(FName(*Origin), 0);
        if (!Environment) return Distances;

        const auto& Systems = Environment->GetRuntimeStarSystems();
        TArray<StarSystem*> Frontier;
        for (StarSystem* System : Systems)
        {
            if (System && FName(ANSI_TO_TCHAR(System->GetName())) == FName(*Origin))
            {
                Frontier.Add(System);
                break;
            }
        }
        for (int32 Index = 0; Index < Frontier.Num(); ++Index)
        {
            StarSystem* From = Frontier[Index];
            const int32 Distance = Distances.FindChecked(FName(ANSI_TO_TCHAR(From->GetName()))) + 1;
            for (StarSystem* To : Systems)
            {
                if (!To) continue;
                const FName Name(ANSI_TO_TCHAR(To->GetName()));
                if (Distances.Contains(Name) || !From->HasLinkTo(To)) continue;
                Distances.Add(Name, Distance);
                Frontier.Add(To);
            }
        }
        return Distances;
    }

    struct FMarketOffer
    {
        FString Name;
        FS_CargoCatalogRow Goods;
        double Available = 0;
        int32 Demand = 0;
        FString Producer;
        double ProducerStock = 0;
        int32 ProducerJumps = MAX_int32;
        int32 PurchaseJumps() const { return Available > 0 ? 0 : ProducerJumps; }
        double Price = 0;
        bool bLocalProduction = false;
        bool bRestricted = false;
        const FString& DisplayName() const { return Goods.DisplayName.IsEmpty() ? Name : Goods.DisplayName; }

        FString Unit() const
        {
            return Goods.Type == ECargoType::Passengers ? TEXT("berths") : Goods.Unit;
        }
    };
    using FOfferPtr = TSharedPtr<FMarketOffer>;

    class SMarketRow : public SMultiColumnTableRow<FOfferPtr>
    {
    public:
        SLATE_BEGIN_ARGS(SMarketRow) {}
            SLATE_ARGUMENT(FOfferPtr, Offer)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
        {
            Offer = Args._Offer;
            SMultiColumnTableRow<FOfferPtr>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner);
        }

        TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
        {
            if (Column == "Item")
                return SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(4,2)[SNew(SShipSystemIcon).Icon(SShipSystemIcon::ForCatalogImage(Offer->Goods.Image))]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Offer->DisplayName()).ToUpper())];
            if (Column == "Illicit")
                return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    .ToolTipText(Offer->Goods.bIllicit ? INVTEXT("Illicit cargo") : INVTEXT("Legal cargo"))
                    [SNew(SCheckBox).IsFocusable(false).Visibility(EVisibility::HitTestInvisible)
                        .IsChecked(Offer->Goods.bIllicit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)];

            FText Text;
            const bool bNumeric = Column == "Available" || Column == "Demand" || Column == "Cost" || Column == "ProducerStock" || Column == "Jumps";
            if (Column == "Item") Text = FText::FromString(Offer->DisplayName());
            else if (Column == "Description") Text = FText::FromString(Offer->Goods.Description);
            else if (Column == "Available")
                Text = FText::AsNumber(Offer->Available);
            else if (Column == "Demand") Text = FText::AsNumber(Offer->Demand);
            else if (Column == "Source")
                Text = Offer->bRestricted ? INVTEXT("RESTRICTED")
                    : Offer->bLocalProduction ? INVTEXT("LOCAL") : INVTEXT("IMPORT");
            else if (Column == "Producer")
                Text = Offer->Producer.IsEmpty() ? INVTEXT("—") : FText::FromString(Offer->Producer);
            else if (Column == "Jumps")
                Text = Offer->PurchaseJumps() == MAX_int32 ? INVTEXT("—") : FText::AsNumber(Offer->PurchaseJumps());
            else if (Column == "ProducerStock")
                Text = Offer->Producer.IsEmpty() ? INVTEXT("—") : FText::AsNumber(Offer->ProducerStock);
            else if (Column == "Rarity") Text = StaticEnum<ESSWAsteroidRarity>()->GetDisplayNameTextByValue(int64(Offer->Goods.Rarity));
            else if (Column == "Unit") Text = FText::FromString(Offer->Unit());
            else if (Column == "Cost")
                Text = Offer->Available > 0
                    ? FText::Format(INVTEXT("{0} cr"), FText::AsNumber(Offer->Price))
                    : INVTEXT("—");

            return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(8, 5))
                .Text(Text.ToUpper()).ToolTipText(Text).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                .Justification(bNumeric ? ETextJustify::Right : ETextJustify::Left);
        }

    private:
        FOfferPtr Offer;
    };

    struct FMarketRecord
    {
        FGuid Id;FString Item,Kind,Status;double Quantity=0,Price=0;int64 Credits=0,Time=0,Sequence=0;
    };
    using FMarketRecordPtr=TSharedPtr<FMarketRecord>;
    class SMarketRecordRow:public SMultiColumnTableRow<FMarketRecordPtr>
    {
    public:
        SLATE_BEGIN_ARGS(SMarketRecordRow){} SLATE_ARGUMENT(FMarketRecordPtr,Record) SLATE_END_ARGS()
        void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
        {Record=Args._Record;SMultiColumnTableRow<FMarketRecordPtr>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner);}
        TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
        {
            FText Text;
            if(Column=="Item")Text=FText::FromString(Record->Item);
            else if(Column=="Kind")Text=FText::FromString(Record->Kind);
            else if(Column=="Quantity")Text=FText::AsNumber(Record->Quantity);
            else if(Column=="Price")Text=FText::AsNumber(Record->Price);
            else if(Column=="Credits")Text=FText::AsNumber(Record->Credits);
            else if(Column=="Status")Text=FText::FromString(Record->Status);
            return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Text.ToUpper()).ToolTipText(Text)
                .Margin(FMargin(8,5)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                .Justification(Column=="Quantity" || Column=="Price" || Column=="Credits"?ETextJustify::Right:ETextJustify::Left);
        }
    private:FMarketRecordPtr Record;
    };
    struct FMarketTable
    {
        TArray<FOfferPtr> Offers;
        TArray<FOfferPtr> Rows;
        TWeakPtr<SListView<FOfferPtr>> List;
        FName SortColumn = "Item";
        EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
        TWeakObjectPtr<UGameInstance> Instance;
        FString System;
        FOfferPtr Selected;
        double Quantity=1;
        FStationMarketQuote BuyQuote,SellQuote;
        FGuid BuyReceipt=FGuid::NewGuid(),SellReceipt=FGuid::NewGuid();
        FText ServiceMessage;
        double NextTradeAt=0;
        int32 View=0;
        int64 LimitPrice=1;
        FGuid SelectedOrder;
        TArray<FMarketRecordPtr> Orders,History;
        TWeakPtr<SListView<FMarketRecordPtr>> OrderList,HistoryList;
        FName RecordSort="Time";
        EColumnSortMode::Type RecordSortMode=EColumnSortMode::Descending;
        void SortRecords()
        {
            const auto Less=[this](const FMarketRecordPtr& A,const FMarketRecordPtr& B)
            {
                const auto& L=*(RecordSortMode==EColumnSortMode::Descending?B:A);const auto& R=*(RecordSortMode==EColumnSortMode::Descending?A:B);
                if(RecordSort=="Item")return L.Item<R.Item;
                if(RecordSort=="Kind")return L.Kind<R.Kind;
                if(RecordSort=="Quantity")return L.Quantity<R.Quantity;
                if(RecordSort=="Price")return L.Price<R.Price;
                if(RecordSort=="Credits")return L.Credits<R.Credits;
                if(RecordSort=="Status")return L.Status<R.Status;
                return L.Time!=R.Time?L.Time<R.Time:L.Sequence<R.Sequence;
            };
            Orders.StableSort(Less);History.StableSort(Less);
            if(auto OrderWidget=OrderList.Pin())OrderWidget->RequestListRefresh();if(auto HistoryWidget=HistoryList.Pin())HistoryWidget->RequestListRefresh();
        }
        void RefreshRecords()
        {
            Orders.Reset();History.Reset();const auto* Simulation=Sim::GetSim();if(!Simulation)return;
            const auto& State=Simulation->ReadEconomy();const FName Local=FEconomyRuntime::Warehouse(System,TEXT("NPC Market"),TEXT("Stock"));
            const auto Name=[this](FName Item)->FString{for(const auto& Offer:Offers)if(FName(*Offer->Name)==Item)return Offer->DisplayName();return Item.ToString();};
            for(const auto& Pair:State.MarketOrders)
            {
                const auto& Order=Pair.Value;if(Order.Owner!=FEconomyLedger::Pilot() || Order.Market!=Local)continue;
                auto Row=MakeShared<FMarketRecord>();Row->Id=Order.Id;Row->Item=Name(Order.Item);Row->Kind=Order.Side==EMarketOrderSide::Buy?TEXT("BUY"):TEXT("SELL");
                Row->Quantity=Order.Remaining;Row->Price=double(Order.UnitPrice);Row->Credits=Order.EscrowCredits;Row->Time=Order.CreatedUtc;Row->Sequence=Order.Sequence;
                Row->Status=Order.State==EMarketOrderState::Open?TEXT("OPEN"):Order.State==EMarketOrderState::Filled?TEXT("FILLED"):Order.State==EMarketOrderState::Expired?TEXT("EXPIRED"):TEXT("CANCELLED");Orders.Add(Row);
            }
            int64 Sequence=0;
            for(const auto& Entry:State.Journal)
            {
                ++Sequence;if(Entry.From!=FEconomyLedger::Pilot() && Entry.To!=FEconomyLedger::Pilot())continue;
                auto Row=MakeShared<FMarketRecord>();Row->Id=Entry.Id;Row->Item=Entry.Item.IsNone()?TEXT("�"):Name(Entry.Item);Row->Kind=Entry.Reason.ToString();
                Row->Quantity=Entry.Quantity;Row->Price=Entry.UnitPrice;Row->Credits=Entry.Credits;Row->Time=Entry.TimestampUtc;Row->Sequence=Sequence;
                Row->Status=Entry.TimestampUtc>0?FDateTime::FromUnixTimestamp(Entry.TimestampUtc).ToString(TEXT("%Y-%m-%d %H:%M UTC")):TEXT("�");History.Add(Row);
            }
            SortRecords();
            if(auto OrderWidget=OrderList.Pin())for(const auto& Row:Orders)if(Row->Id==SelectedOrder){OrderWidget->SetSelection(Row);break;}
        }
        bool CanManageOrder() const
        {
            const auto* Simulation=Sim::GetSim();const auto* Order=Simulation?Simulation->ReadEconomy().MarketOrders.Find(SelectedOrder):nullptr;
            return FStationServices::IsDocked() && Order && Order->Owner==FEconomyLedger::Pilot() && Order->State==EMarketOrderState::Open;
        }
        void CrossNPC(FEconomyState& State,FGuid Id,int64 Ask,int64 Bid,int64 Now)
        {
            const auto* Found=State.MarketOrders.Find(Id);if(!Found || Found->State!=EMarketOrderState::Open)return;
            const auto Order=*Found;const auto* NPC=State.Inventories.Find(Order.Market);if(!NPC)return;
            const FName Owner=NPC->Owner;
            double Count=0;int64 Price=0;const auto Side=Order.Side==EMarketOrderSide::Buy?EMarketOrderSide::Sell:EMarketOrderSide::Buy;
            if(Order.Side==EMarketOrderSide::Buy && Ask>0 && Order.UnitPrice>=Ask)
            {Price=Ask;Count=FMath::Min(Order.Remaining,FMath::FloorToDouble(FEconomyLedger::Available(State,Order.Market,Order.Item)));}
            else if(Order.Side==EMarketOrderSide::Sell && Bid>0 && Order.UnitPrice<=Bid)
            {
                Price=Bid;Count=FMath::Min3(Order.Remaining,FMath::FloorToDouble(double(State.Wallets.FindRef(Owner))/Price),
                    FMath::FloorToDouble(FMath::Max(0.0,NPC->CapacityTonnes-FEconomyLedger::Used(*NPC))/Order.UnitTonnes));
            }
            if(Count<=0)return;
            FText Reason;const FGuid Counter=FEconomyLedger::PlaceMarketOrder(State,Owner,Order.Market,Order.Market,Order.Item,Side,Count,Order.UnitTonnes,Price,Now,Now+60,FGuid::NewGuid(),Reason);
            FEconomyLedger::AdvanceMarketOrders(State,Now);
            if(Counter.IsValid())
                if(const auto* Remaining=State.MarketOrders.Find(Counter);Remaining && Remaining->State==EMarketOrderState::Open)
                    FEconomyLedger::CancelMarketOrder(State,Owner,Counter,FGuid::NewGuid(),Reason);
        }
        void OrderAction(int32 Action)
        {
            if(!FStationServices::IsDocked() || (Action>=2 && !CanManageOrder()) || (Action<2 && (!Selected || Selected->bRestricted || Selected->Goods.Type==ECargoType::Passengers)))return;
            auto* Simulation=Sim::GetSim();auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
            auto* Players=Instance.IsValid()?Instance->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;if(!Craft || !Players)return;
            Craft->SyncCargoInventory();
            const auto* Existing=Simulation->ReadEconomy().MarketOrders.Find(SelectedOrder);
            const FName Item=Action<2?FName(*Selected->Name):Existing->Item;
            const double Count=Action<2?Quantity:Existing->Remaining;
            FStationMarketQuote AskQuote,BidQuote;
            FStationServices::QuoteMarket(Instance.Get(),Item,Count,true,AskQuote);FStationServices::QuoteMarket(Instance.Get(),Item,Count,false,BidQuote);
            const int64 Ask=FMath::IsFinite(AskQuote.UnitPrice) && AskQuote.UnitPrice>0?FMath::CeilToInt64(AskQuote.UnitPrice):0;
            const int64 Bid=FMath::IsFinite(BidQuote.UnitPrice) && BidQuote.UnitPrice>0?FMath::FloorToInt64(BidQuote.UnitPrice):0;
            auto& State=Simulation->GetEconomy();const auto Before=State;const int64 Now=FDateTime::UtcNow().ToUnixTimestamp();bool Changed=false;FGuid Id=SelectedOrder;
            if(Action<2)
            {
                Id=FEconomyLedger::PlaceMarketOrder(State,FEconomyLedger::Pilot(),FEconomyRuntime::Warehouse(System,TEXT("NPC Market"),TEXT("Stock")),Craft->GetCargoInventoryKey(),Item,
                    Action==0?EMarketOrderSide::Buy:EMarketOrderSide::Sell,Quantity,Selected->Goods.Size,LimitPrice,Now,Now+24*3600,FGuid::NewGuid(),ServiceMessage);
                Changed=Id.IsValid();
            }
            else if(Action==2)Changed=FEconomyLedger::ModifyMarketOrder(State,FEconomyLedger::Pilot(),Id,LimitPrice,FGuid::NewGuid(),ServiceMessage);
            else Changed=FEconomyLedger::CancelMarketOrder(State,FEconomyLedger::Pilot(),Id,FGuid::NewGuid(),ServiceMessage);
            if(!Changed)return;
            NextTradeAt=FPlatformTime::Seconds()+.35;
            if(Action<3)CrossNPC(State,Id,Ask,Bid,Now);
            FEconomyLedger::AdvanceMarketOrders(State,Now);
            Craft->ApplyCargoInventoryToManifest();
            if(!Players->SaveActivePlayerProgress())
            {State=Before;Craft->ApplyCargoInventoryToManifest();ServiceMessage=INVTEXT("COULD NOT SAVE THE ORDER CHANGE. CARGO, CREDITS AND RESERVATIONS WERE RESTORED.");}
            else {Craft->ApplyCargoInventoryToManifest();SelectedOrder=Id;ServiceMessage=INVTEXT("ORDER CHANGE SAVED. MARKETABLE ORDERS MATCH REAL STOCK AND FUNDED COUNTERPARTS; OPEN REMAINDERS STAY RESERVED.");}
            RefreshLive();
        }

        void UpdateQuotes()
        {
            BuyQuote=FStationMarketQuote{};SellQuote=FStationMarketQuote{};
            if(!Selected)return;
            FStationServices::QuoteMarket(Instance.Get(),FName(*Selected->Name),Quantity,true,BuyQuote);
            FStationServices::QuoteMarket(Instance.Get(),FName(*Selected->Name),Quantity,false,SellQuote);
        }
        void ChangeOrder()
        {BuyReceipt=FGuid::NewGuid();SellReceipt=FGuid::NewGuid();ServiceMessage=FText::GetEmpty();UpdateQuotes();}
        void RefreshLive()
        {
            const auto* Simulation=Sim::GetSim();
            if(Simulation)
            {
                const auto& State=Simulation->ReadEconomy();
                const FName Local=FEconomyRuntime::Warehouse(System,TEXT("NPC Market"),TEXT("Stock"));
                for(auto& Offer:Offers)
                {
                    Offer->Available=Offer->bRestricted?0:FEconomyLedger::Available(State,Local,FName(*Offer->Name));
                    const double Pressure=double(Offer->Demand)/FMath::Max(1.0,Offer->Available);
                    Offer->Price=Offer->Goods.Value*FMath::Clamp(.8+Pressure,.85,3.0);
                    if(!Offer->Producer.IsEmpty() && Offer->Goods.Size>0)
                        Offer->ProducerStock=FEconomyLedger::SystemQuantity(State,FName(*Offer->Producer),FName(*Offer->Name))/Offer->Goods.Size;
                }
            }
            UpdateQuotes();Sort();RefreshRecords();
        }
        bool CanTrade(bool Buy) const
        {
            const auto* Simulation=Sim::GetSim();const FGuid Receipt=Buy?BuyReceipt:SellReceipt;
            return FPlatformTime::Seconds()>=NextTradeAt && Selected && (Buy?BuyQuote.bAllowed:SellQuote.bAllowed) && Simulation && !Simulation->ReadEconomy().CommittedTransactions.Contains(Receipt);
        }
        void Trade(bool Buy)
        {
            if(!CanTrade(Buy))return;
            const auto Quote=Buy?BuyQuote:SellQuote;
            if(FStationServices::TradeMarket(Instance.Get(),FName(*Selected->Name),Quantity,Buy,Quote.TotalCredits,Buy?BuyReceipt:SellReceipt,ServiceMessage))
            {
                BuyReceipt=FGuid::NewGuid();SellReceipt=FGuid::NewGuid();
                NextTradeAt=FPlatformTime::Seconds()+.35;
            }
            RefreshLive();
        }

        void Filter(const FText& Search)
        {
            const FString Query = Search.ToString().TrimStartAndEnd();
            Rows.Reset();
            if (Query == TEXT("*"))
                Rows = Offers;
            else if (!Query.IsEmpty())
            {
                for (const auto& Offer : Offers)
                    if (Offer->DisplayName().Contains(Query, ESearchCase::IgnoreCase))
                        Rows.Add(Offer);
            }
            Sort();
        }

        void Sort()
        {
            Rows.Sort([this](const FOfferPtr& Left, const FOfferPtr& Right)
            {
                const auto& A = *(SortMode == EColumnSortMode::Descending ? Right : Left);
                const auto& B = *(SortMode == EColumnSortMode::Descending ? Left : Right);
                if (SortColumn == "Jumps" && A.PurchaseJumps() != B.PurchaseJumps()) return A.PurchaseJumps() < B.PurchaseJumps();
                if (SortColumn == "ProducerStock" && A.ProducerStock != B.ProducerStock) return A.ProducerStock < B.ProducerStock;
                if (SortColumn == "Available" && A.Available != B.Available) return A.Available < B.Available;
                if (SortColumn == "Demand" && A.Demand != B.Demand) return A.Demand < B.Demand;
                if (SortColumn == "Illicit" && A.Goods.bIllicit != B.Goods.bIllicit) return A.Goods.bIllicit < B.Goods.bIllicit;
                if (SortColumn == "Rarity" && A.Goods.Rarity != B.Goods.Rarity) return A.Goods.Rarity < B.Goods.Rarity;
                if (SortColumn == "Unit")
                {
                    const int32 Order = A.Unit().Compare(B.Unit(), ESearchCase::IgnoreCase);
                    if (Order != 0) return Order < 0;
                }
                if (SortColumn == "Cost" && A.Price != B.Price) return A.Price < B.Price;
                if (SortColumn == "Description")
                {
                    const int32 Order = A.Goods.Description.Compare(B.Goods.Description, ESearchCase::IgnoreCase);
                    if (Order != 0) return Order < 0;
                }
                const int32 LabelOrder = A.DisplayName().Compare(B.DisplayName(), ESearchCase::IgnoreCase);
                return LabelOrder != 0 ? LabelOrder < 0 : A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
            });
            if (auto OffersWidget = List.Pin()) OffersWidget->RequestListRefresh();
        }
    };
}

TSharedRef<SWidget> UMarketplaceDlg::RebuildWidget()
{
    const auto Table = MakeShared<FMarketTable>();
    Table->Instance=GetGameInstance();
    Sim* Simulation = Sim::GetSim();
    const Mission* CurrentMission = Simulation ? Simulation->GetMission() : nullptr;
    FString System;
    if (SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr)
        if (Region->GetSystem()) System = ANSI_TO_TCHAR(Region->GetSystem()->GetName());
    if (System.IsEmpty() && CurrentMission)
    {
        if (const char* Name = CurrentMission->GetSystem())
            System = FString(UTF8_TO_TCHAR(Name)).TrimStartAndEnd();
        if (System.IsEmpty() && CurrentMission->GetStarSystem())
            System = ANSI_TO_TCHAR(CurrentMission->GetStarSystem()->GetName());
    }
    Table->System=System;
    const auto Markets = LoadSystemMarkets();
    const int32 LocalIndex = Markets.IndexOfByPredicate([&System](const FSystemMarket& Market)
    {
        return Market.Name.Equals(System, ESearchCase::IgnoreCase);
    });
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (const auto* Catalog = Instance->GetSubsystem<UCargoCatalogSubsystem>())
        {
            const auto Distances = GetJumpDistances(Instance->GetSubsystem<UStarshatterEnvironmentSubsystem>(), System);
            Catalog->ForEachDefinition([Table, &Markets, &Distances, LocalIndex](FName Name, const FS_CargoCatalogRow& Goods)
            {
                if (LocalIndex == INDEX_NONE) return;
                auto Offer = MakeShared<FMarketOffer>();
                Offer->Name = Name.ToString();
                Offer->Goods = Goods;
                const int32 ExclusiveIndex = Markets.IndexOfByPredicate([Name](const FSystemMarket& Market)
                {
                    return Market.ExclusiveSupply.Contains(Name);
                });
                Offer->bRestricted = ExclusiveIndex != INDEX_NONE
                    ? LocalIndex != ExclusiveIndex : Goods.bIllicit && !Markets[LocalIndex].bIllicitTrade;
                const int32 BaseStock = FMath::Max(1, FMath::Min(Goods.PreviewQuantity,
                    160 >> FMath::Clamp(int32(Goods.Rarity),0,7)));
                TArray<int32> Stock, Demand, Production;
                const auto* Simulation = Sim::GetSim();
                const auto* Economy = Simulation ? &Simulation->ReadEconomy() : nullptr;
                for (int32 Index = 0; Index < Markets.Num(); ++Index)
                {
                    const auto& Market = Markets[Index];
                    const bool bTrades = ExclusiveIndex != INDEX_NONE ? Index == ExclusiveIndex : !Goods.bIllicit || Market.bIllicitTrade;
                    const bool bProduces = Market.Production.Contains(Name) || Index == ExclusiveIndex;
                    Demand.Add(bTrades ? FMath::Max(1, FMath::RoundToInt(BaseStock * 0.15 * Market.DemandScale)) : 0);
                    Production.Add(bProduces && bTrades ? BaseStock : 0);
                    const double Tonnes = Economy ? FEconomyLedger::SystemQuantity(*Economy, FName(*Market.Name), Name) : 0;
                    Stock.Add(Goods.Size > 0 ? static_cast<int32>(FMath::Clamp(FMath::FloorToDouble(Tonnes / Goods.Size), 0.0, double(MAX_int32))) : 0);
                }
                Offer->Available = Stock[LocalIndex];
                Offer->Demand = Demand[LocalIndex];
                Offer->bLocalProduction = Production[LocalIndex] > 0;
                const double Pressure = double(Offer->Demand) / FMath::Max(1.0, Offer->Available);
                Offer->Price = Goods.Value * FMath::Clamp(0.8 + Pressure, 0.85, 3.0);
                for (int32 Index = 0; Index < Markets.Num(); ++Index)
                {
                    if (Stock[Index] <= 0) continue;
                    const FString& MarketName = Markets[Index].Name;
                    const int32* Jumps = Distances.Find(FName(*MarketName));
                    if (!Jumps || *Jumps > Offer->ProducerJumps) continue;
                    if (*Jumps == Offer->ProducerJumps && Stock[Index] <= Offer->ProducerStock) continue;
                    Offer->Producer = MarketName;
                    Offer->ProducerJumps = *Jumps;
                    Offer->ProducerStock = Stock[Index];
                }
                Table->Offers.Add(MoveTemp(Offer));
            });
        }
    }
    auto Header = SNew(SHeaderRow);
    const auto AddColumn = [Header, Table](FName Id, const FText& Label, float Width)
    {
        Header->AddColumn(SHeaderRow::Column(Id).DefaultLabel(Label.ToUpper()).FillWidth(Width)
            .SortMode_Lambda([Table, Id]()
            {
                return Table->SortColumn == Id ? Table->SortMode : EColumnSortMode::None;
            })
            .OnSort_Lambda([Table](EColumnSortPriority::Type, const FName& Column, EColumnSortMode::Type Mode)
            {
                Table->SortColumn = Column;
                Table->SortMode = Mode;
                Table->Sort();
            })
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Label.ToUpper())]);
    };
    AddColumn("Item", INVTEXT("Item"), 2.f);
    AddColumn("Description", INVTEXT("Description"), 4.f);
    AddColumn("Available", INVTEXT("NPC stock"), 1.1f);
    AddColumn("Demand", INVTEXT("Demand / hr"), 1.2f);
    AddColumn("Producer", INVTEXT("Nearest producer"), 2.f);
    AddColumn("Jumps", INVTEXT("Jumps"), 0.8f);
    AddColumn("ProducerStock", INVTEXT("Producer stock"), 1.4f);
    AddColumn("Source", INVTEXT("Local status"), 1.3f);
    AddColumn("Rarity", INVTEXT("RARITY"), 1.1f);
    AddColumn("Illicit", INVTEXT("ILLICIT"), 0.8f);
    AddColumn("Unit", INVTEXT("Unit"), 1.f);
    AddColumn("Cost", INVTEXT("Cost / unit"), 1.3f);

    const auto Selected = MakeShared<FOfferPtr>();
    auto List = SNew(SListView<FOfferPtr>).ListItemsSource(&Table->Rows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::Single)
        .OnSelectionChanged_Lambda([Selected,Table](FOfferPtr Offer,ESelectInfo::Type)
        { *Selected=Offer;Table->Selected=MoveTemp(Offer);Table->Quantity=1;Table->LimitPrice=Table->Selected?FMath::Max<int64>(1,FMath::CeilToInt64(Table->Selected->Price)):1;Table->ChangeOrder(); })
        .OnGenerateRow_Lambda([Table](FOfferPtr Offer, const TSharedRef<STableViewBase>& Owner)
            -> TSharedRef<ITableRow>
        {
            return SNew(SMarketRow, Owner).Offer(Offer);
        });
    Table->List = List;
    const auto Records=[Table](bool History)->TSharedRef<SListView<FMarketRecordPtr>>
    {
        auto Columns=SNew(SHeaderRow);
        const auto Column=[Table,Columns](FName Id,const FText& Label,float Width)
        {
            Columns->AddColumn(SHeaderRow::Column(Id).DefaultLabel(Label).FillWidth(Width)
                .SortMode_Lambda([Table,Id](){return Table->RecordSort==Id?Table->RecordSortMode:EColumnSortMode::None;})
                .OnSort_Lambda([Table](EColumnSortPriority::Type,const FName& Id,EColumnSortMode::Type Mode){Table->RecordSort=Id;Table->RecordSortMode=Mode;Table->SortRecords();}));
        };
        Column("Item",INVTEXT("ITEM"),2.2);Column("Kind",History?INVTEXT("TRANSACTION"):INVTEXT("SIDE"),1.5);
        Column("Quantity",INVTEXT("QUANTITY"),1);Column("Price",INVTEXT("CR / UNIT"),1);
        Column("Credits",History?INVTEXT("CREDITS"):INVTEXT("ESCROW CR"),1.2);Column("Status",History?INVTEXT("UTC"):INVTEXT("STATUS"),1.8);
        return SNew(SListView<FMarketRecordPtr>).ListItemsSource(History?&Table->History:&Table->Orders).HeaderRow(Columns)
            .SelectionMode(ESelectionMode::Single)
            .OnSelectionChanged_Lambda([Table,History](FMarketRecordPtr Row,ESelectInfo::Type)
            {if(!History && Row){Table->SelectedOrder=Row->Id;Table->LimitPrice=FMath::Max<int64>(1,static_cast<int64>(Row->Price));}})
            .OnGenerateRow_Lambda([](FMarketRecordPtr Row,const TSharedRef<STableViewBase>& Owner)->TSharedRef<ITableRow>
            {return SNew(SMarketRecordRow,Owner).Record(Row);});
    };
    auto Orders=Records(false),History=Records(true);Table->OrderList=Orders;Table->HistoryList=History;
    Table->RefreshLive();
    List->RegisterActiveTimer(.5f,FWidgetActiveTimerDelegate::CreateLambda([Table](double,float)
    {Table->RefreshLive();return EActiveTimerReturnType::Continue;}));
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(20, 18, 20, 10)
        [
            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13))
            .Text(Table->Offers.IsEmpty() ? INVTEXT("NO MARKET DATA")
                : FText::Format(INVTEXT("{0} MARKET"), FText::FromString(System).ToUpper()))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(20, 0, 20, 10)
        [
            SNew(SSearchBox)
            .HintText(INVTEXT("Search item names, or type * to show all"))
            .OnTextChanged_Lambda([Table, Selected](const FText& Search)
            {
                if (auto OffersWidget = Table->List.Pin()) OffersWidget->ClearSelection();
                Selected->Reset();Table->Selected.Reset();Table->ChangeOrder();
                Table->Filter(Search);
            })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(20,0,20,8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).ToolTipText(INVTEXT("Local NPC goods"))
                .OnClicked_Lambda([Table](){Table->View=0;return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Marketplace)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).ToolTipText(INVTEXT("My local limit orders"))
                .OnClicked_Lambda([Table](){Table->View=1;Table->RefreshRecords();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Contracts)]]
            + SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Recent saved transactions"))
                .OnClicked_Lambda([Table](){Table->View=2;Table->RefreshRecords();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Intel)]]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(10,0)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text_Lambda([Table](){return Table->View==0?INVTEXT("NPC MARKET"):Table->View==1?INVTEXT("MY LIMIT ORDERS � SELECT AN OPEN ROW TO REPRICE OR CANCEL"):INVTEXT("TRANSACTION HISTORY � MOST RECENT 512 EVENTS");})]]
        + SVerticalBox::Slot().FillHeight(0.65f).Padding(20, 0, 20, 8)
        [MissionUIStyle::MakeDataPane(SNew(SOverlay)
            + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Table](){return Table->View==0?EVisibility::Visible:EVisibility::Collapsed;})[List]]
            + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Table](){return Table->View==1?EVisibility::Visible:EVisibility::Collapsed;})[Orders]]
            + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([Table](){return Table->View==2?EVisibility::Visible:EVisibility::Collapsed;})[History]],FMargin(0))]
        + SVerticalBox::Slot().FillHeight(0.35f).Padding(20, 0, 20, 18)
        [
            MissionUIStyle::MakeDataPane(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().FillHeight(1)
                [SNew(SScrollBox) + SScrollBox::Slot()
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
                        .Text_Lambda([Selected]()
                        {
                            if(!*Selected)return INVTEXT("SELECT AN ITEM TO VIEW MARKET DETAILS. TRADING REQUIRES DOCKING.");
                            const auto& Offer=**Selected;
                            const FText Source=Offer.Producer.IsEmpty()?INVTEXT("NO REACHABLE PRODUCER WITH STOCK"):
                                FText::Format(INVTEXT("{0} � {1} JUMPS � {2} {3} IN STOCK"),FText::FromString(Offer.Producer).ToUpper(),
                                    FText::AsNumber(Offer.ProducerJumps),FText::AsNumber(Offer.ProducerStock),FText::FromString(Offer.Unit()).ToUpper());
                            return FText::Format(INVTEXT("{0}\n{1}\n{2} � {3} {4} LOCAL NPC STOCK\n{5}"),
                                FText::FromString(Offer.DisplayName()).ToUpper(),FText::FromString(Offer.Goods.Description),
                                Offer.Goods.bIllicit?INVTEXT("ILLICIT"):INVTEXT("LEGAL"),FText::AsNumber(Offer.Available),FText::FromString(Offer.Unit()).ToUpper(),Source);
                        })]]
                + SVerticalBox::Slot().AutoHeight().Padding(0,8)
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(INVTEXT("QUANTITY"))]
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SBox).WidthOverride(130)[SNew(SSpinBox<double>).MinValue(.001).MaxValue(1.e12)
                        .MinFractionalDigits(0).MaxFractionalDigits(3).Delta(.1)
                        .Value_Lambda([Table](){return Table->Quantity;})
                        .OnValueChanged_Lambda([Table](double Value){Table->Quantity=Value;Table->ChangeOrder();})
                        .ToolTipText(INVTEXT("Quantity to buy or sell. Goods use whole units; tonnes may be fractional."))]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Buy the quoted quantity into ship cargo"))
                        .IsEnabled_Lambda([Table](){return Table->CanTrade(true);})
                        .OnClicked_Lambda([Table](){Table->Trade(true);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Down)]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Sell the quoted quantity from ship cargo"))
                        .IsEnabled_Lambda([Table](){return Table->CanTrade(false);})
                        .OnClicked_Lambda([Table](){Table->Trade(false);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Up)]]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(8,0)
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
                        .Text_Lambda([](){const auto* Simulation=Sim::GetSim();return FText::Format(INVTEXT("BALANCE  {0} CR"),FText::AsNumber(Simulation?Simulation->ReadEconomy().Wallets.FindRef(FEconomyLedger::Pilot()):0));})]]
                + SVerticalBox::Slot().AutoHeight().Padding(0,2)
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(INVTEXT("LIMIT CR / UNIT"))]
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SBox).WidthOverride(140)[SNew(SSpinBox<int64>).MinValue(1).MaxValue(1000000000000LL).Delta(1)
                        .Value_Lambda([Table](){return Table->LimitPrice;}).OnValueChanged_Lambda([Table](int64 Value){Table->LimitPrice=Value;})
                        .ToolTipText(INVTEXT("Queued orders use whole units/tonnes and whole-credit unit prices. Orders expire after 24 hours; immediate NPC trades still accept fractional tonnes."))]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Place a 24-hour buy limit order; reserves real credits and cargo space"))
                        .IsEnabled_Lambda([Table](){return Table->Selected && !Table->Selected->bRestricted && FStationServices::IsDocked() && FPlatformTime::Seconds()>=Table->NextTradeAt;})
                        .OnClicked_Lambda([Table](){Table->OrderAction(0);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Down)]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Place a 24-hour sell limit order; reserves actual ship cargo"))
                        .IsEnabled_Lambda([Table](){return Table->Selected && !Table->Selected->bRestricted && FStationServices::IsDocked() && FPlatformTime::Seconds()>=Table->NextTradeAt;})
                        .OnClicked_Lambda([Table](){Table->OrderAction(1);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Up)]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Reprice the selected open order; changes escrow and resets time priority"))
                        .IsEnabled_Lambda([Table](){return Table->CanManageOrder();})
                        .OnClicked_Lambda([Table](){Table->OrderAction(2);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Refresh)]]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0)
                    [SNew(SButton).ToolTipText(INVTEXT("Cancel the selected open order and release unspent escrow or unsold cargo"))
                        .IsEnabled_Lambda([Table](){return Table->CanManageOrder();})
                        .OnClicked_Lambda([Table](){Table->OrderAction(3);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]]
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([Table]()
                {return Table->Selected?FText::Format(INVTEXT("{0}  |  MAX {1}\n{2}  |  MAX {3}"),Table->BuyQuote.Reason,FText::AsNumber(Table->BuyQuote.Maximum),Table->SellQuote.Reason,FText::AsNumber(Table->SellQuote.Maximum)):FText::GetEmpty();})]
                + SVerticalBox::Slot().AutoHeight().Padding(0,4)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([Table](){return Table->ServiceMessage;})]
            )
        ];
}
