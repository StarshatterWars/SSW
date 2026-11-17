#include "MarketplaceDlg.h"

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
        int32 Available = 0;
        int32 Demand = 0;
        FString Producer;
        int32 ProducerStock = 0;
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
            SMultiColumnTableRow<FOfferPtr>::Construct(FSuperRowType::FArguments(), Owner);
        }

        TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
        {
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

    struct FMarketTable
    {
        TArray<FOfferPtr> Offers;
        TArray<FOfferPtr> Rows;
        TWeakPtr<SListView<FOfferPtr>> List;
        FName SortColumn = "Item";
        EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;

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
            if (auto View = List.Pin()) View->RequestListRefresh();
        }
    };
}

TSharedRef<SWidget> UMarketplaceDlg::RebuildWidget()
{
    const auto Table = MakeShared<FMarketTable>();
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
    const auto Markets = LoadSystemMarkets();
    const int32 LocalIndex = Markets.IndexOfByPredicate([&System](const FSystemMarket& Market)
    {
        return Market.Name.Equals(System, ESearchCase::IgnoreCase);
    });
    const int32 Hours = Simulation ? FMath::Clamp(FMath::FloorToInt(Simulation->MissionClock() / 3600.0), 0, 168) : 0;
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (const auto* Catalog = Instance->GetSubsystem<UCargoCatalogSubsystem>())
        {
            const auto Distances = GetJumpDistances(Instance->GetSubsystem<UStarshatterEnvironmentSubsystem>(), System);
            Catalog->ForEachDefinition([Table, &Markets, &Distances, LocalIndex, Hours](FName Name, const FS_CargoCatalogRow& Goods)
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
                    160 >> static_cast<uint8>(Goods.Rarity)));
                TArray<int32> Stock, Demand, Production;
                for (int32 Index = 0; Index < Markets.Num(); ++Index)
                {
                    const auto& Market = Markets[Index];
                    const bool bTrades = ExclusiveIndex != INDEX_NONE
                        ? Index == ExclusiveIndex : !Goods.bIllicit || Market.bIllicitTrade;
                    const bool bProduces = Market.Production.Contains(Name) || Index == ExclusiveIndex;
                    const int32 Need = bTrades ? FMath::Max(1, FMath::RoundToInt(BaseStock * 0.15 * Market.DemandScale)) : 0;
                    Demand.Add(Need);
                    Production.Add(bProduces && bTrades ? BaseStock : 0);
                    Stock.Add(bProduces && bTrades ? BaseStock * 2 : 0);
                }
                // Model hourly use, production and abstract intersystem deliveries.
                for (int32 Hour = 0; Hour < Hours; ++Hour)
                {
                    for (int32 Index = 0; Index < Markets.Num(); ++Index)
                        Stock[Index] = FMath::Clamp(Stock[Index] - Demand[Index] + Production[Index], 0, BaseStock * 4);
                    for (int32 Destination = 0; Destination < Markets.Num(); ++Destination)
                    {
                        int32 Deficit = Demand[Destination] - Stock[Destination];
                        for (int32 Source = 0; Source < Markets.Num() && Deficit > 0; ++Source)
                        {
                            if (Source == Destination || Production[Source] == 0) continue;
                            const int32 Shipment = FMath::Min3(Deficit, FMath::Max(0, Stock[Source] - 2 * Demand[Source]),
                                FMath::Max(1, Production[Source] / 8));
                            Stock[Source] -= Shipment;
                            Stock[Destination] += Shipment;
                            Deficit -= Shipment;
                        }
                    }
                }
                Offer->Available = Stock[LocalIndex];
                Offer->Demand = Demand[LocalIndex];
                Offer->bLocalProduction = Production[LocalIndex] > 0;
                const double Pressure = double(Offer->Demand) / FMath::Max(1, Offer->Available);
                Offer->Price = Goods.Value * FMath::Clamp(0.8 + Pressure, 0.85, 3.0);
                for (int32 Index = 0; Index < Markets.Num(); ++Index)
                {
                    if (Production[Index] <= 0 || Stock[Index] <= 0) continue;
                    const FString& MarketName = Markets[Index].Name;
                    const int32* Jumps = Distances.Find(FName(*MarketName));
                    if (!Jumps || *Jumps > Offer->ProducerJumps) continue;
                    if (*Jumps == Offer->ProducerJumps &&
                        (Stock[Index] < Offer->ProducerStock ||
                            (Stock[Index] == Offer->ProducerStock && MarketName.Compare(Offer->Producer, ESearchCase::IgnoreCase) >= 0)))
                        continue;

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
    AddColumn("Available", INVTEXT("Quantity"), 1.1f);
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
        .OnSelectionChanged_Lambda([Selected](FOfferPtr Offer, ESelectInfo::Type) { *Selected = MoveTemp(Offer); })
        .OnGenerateRow_Lambda([Table](FOfferPtr Offer, const TSharedRef<STableViewBase>& Owner)
            -> TSharedRef<ITableRow>
        {
            return SNew(SMarketRow, Owner).Offer(Offer);
        });
    Table->List = List;
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
                if (auto View = Table->List.Pin()) View->ClearSelection();
                Selected->Reset();
                Table->Filter(Search);
            })
        ]
        + SVerticalBox::Slot().FillHeight(0.7f).Padding(20, 0, 20, 8)
        [MissionUIStyle::MakeDataPane(List, FMargin(0))]
        + SVerticalBox::Slot().FillHeight(0.3f).Padding(20, 0, 20, 18)
        [
            MissionUIStyle::MakeDataPane(
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).AutoWrapText(true)
                .Text_Lambda([Selected]()
                {
                    if (!*Selected) return INVTEXT("Select an item to view market details.");
                    const auto& Offer = **Selected;
                    const FText Source = Offer.Producer.IsEmpty() ? INVTEXT("No reachable producer with stock") :
                        FText::Format(INVTEXT("{0} · {1} jumps · {2} {3} in stock"),
                            FText::FromString(Offer.Producer), FText::AsNumber(Offer.ProducerJumps),
                            FText::AsNumber(Offer.ProducerStock), FText::FromString(Offer.Unit()));
                    return FText::Format(INVTEXT("{0}\n{1}\n{2} · {3} {4} available locally\n{5}"),
                        FText::FromString(Offer.DisplayName()).ToUpper(), FText::FromString(Offer.Goods.Description),
                        Offer.Goods.bIllicit ? INVTEXT("ILLICIT") : INVTEXT("LEGAL"),
                        FText::AsNumber(Offer.Available), FText::FromString(Offer.Unit()), Source);
                })
                ])
        ];
}
