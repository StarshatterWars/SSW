#include "MarketplaceDlg.h"

#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameStructs.h"
#include "MissionUIStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
    struct FMarketOffer
    {
        FString Name;
        FS_CargoCatalogRow Goods;
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
            FText Text;
            const bool bNumeric = Column == "Available" || Column == "Cost";
            if (Column == "Item") Text = FText::FromString(Offer->DisplayName());
            else if (Column == "Description") Text = FText::FromString(Offer->Goods.Description);
            else if (Column == "Available")
                Text = FText::AsNumber(Offer->Goods.PreviewQuantity);
            else if (Column == "Rarity") Text = StaticEnum<ESSWAsteroidRarity>()->GetDisplayNameTextByValue(int64(Offer->Goods.Rarity));
            else if (Column == "Unit") Text = FText::FromString(Offer->Unit());
            else if (Column == "Cost")
                Text = FText::Format(INVTEXT("{0} cr"), FText::AsNumber(Offer->Goods.Value));

            return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Margin(FMargin(8, 5))
                .Text(Text.ToUpper()).ToolTipText(Text).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                .Justification(bNumeric ? ETextJustify::Right : ETextJustify::Left);
        }

    private:
        FOfferPtr Offer;
    };

    struct FMarketTable
    {
        TArray<FOfferPtr> Rows;
        TWeakPtr<SListView<FOfferPtr>> List;
        FName SortColumn = "Item";
        EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;

        void Sort()
        {
            Rows.Sort([this](const FOfferPtr& Left, const FOfferPtr& Right)
            {
                const auto& A = *(SortMode == EColumnSortMode::Descending ? Right : Left);
                const auto& B = *(SortMode == EColumnSortMode::Descending ? Left : Right);
                if (SortColumn == "Available" && A.Goods.PreviewQuantity != B.Goods.PreviewQuantity)
                    return A.Goods.PreviewQuantity < B.Goods.PreviewQuantity;
                if (SortColumn == "Rarity" && A.Goods.Rarity != B.Goods.Rarity) return A.Goods.Rarity < B.Goods.Rarity;
                if (SortColumn == "Unit")
                {
                    const int32 Order = A.Unit().Compare(B.Unit(), ESearchCase::IgnoreCase);
                    if (Order != 0) return Order < 0;
                }
                if (SortColumn == "Cost" && A.Goods.Value != B.Goods.Value) return A.Goods.Value < B.Goods.Value;
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
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (const auto* Catalog = Instance->GetSubsystem<UCargoCatalogSubsystem>())
        {
            Catalog->ForEachDefinition([Table](FName Name, const FS_CargoCatalogRow& Goods)
            {
                auto Offer = MakeShared<FMarketOffer>();
                Offer->Name = Name.ToString();
                Offer->Goods = Goods;
                Table->Rows.Add(MoveTemp(Offer));
            });
        }
    }
    Table->Sort();

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
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(Label.ToUpper())]);
    };
    AddColumn("Item", INVTEXT("Item"), 2.f);
    AddColumn("Description", INVTEXT("Description"), 4.f);
    AddColumn("Available", INVTEXT("Quantity"), 1.1f);
    AddColumn("Rarity", INVTEXT("RARITY"), 1.1f);
    AddColumn("Unit", INVTEXT("Unit"), 1.f);
    AddColumn("Cost", INVTEXT("Cost / unit"), 1.3f);

    auto List = SNew(SListView<FOfferPtr>).ListItemsSource(&Table->Rows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::Single)
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
            .Text(Table->Rows.IsEmpty() ? INVTEXT("NO GOODS AVAILABLE")
                : FText::Format(INVTEXT("{0} CATALOG ENTRIES  ·  PREVIEW STOCK"), FText::AsNumber(Table->Rows.Num())))
        ]
        + SVerticalBox::Slot().FillHeight(1).Padding(20, 0, 20, 18)
        [ List ];
}
