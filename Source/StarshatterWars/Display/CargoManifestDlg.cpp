#include "CargoManifestDlg.h"
#include "StationServices.h"
#include "Algo/AllOf.h"
#include "MissionUIStyle.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipSystemIcon.h"
#include "Sim.h"
#include "CargoCatalogSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    Ship* CargoShip()
    {
        Sim* Simulation = Sim::GetSim();
        Ship* Pilot = Simulation ? Simulation->GetPlayerShip() : nullptr;
        return Pilot && !Pilot->IsDead() ? Pilot : nullptr;
    }

    using FCargoItem = TSharedPtr<FS_Cargo>;

    class SCargoRow : public SMultiColumnTableRow<FCargoItem>
    {
    public:
        SLATE_BEGIN_ARGS(SCargoRow) {}
            SLATE_ARGUMENT(FCargoItem, Item)
        SLATE_END_ARGS()

        void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
        {
            Item = Args._Item;
            SMultiColumnTableRow<FCargoItem>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner);
        }

        TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
        {
            if (Column == "Name")
                return SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(4,2)[SNew(SShipSystemIcon).Icon(SShipSystemIcon::ForCatalogImage(Item->Image))]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Item->DisplayName).ToUpper())];
            if (Column == "Illicit")
                return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
                    .ToolTipText(Item->bIllicit ? INVTEXT("Illicit cargo") : INVTEXT("Legal cargo"))
                    [SNew(SCheckBox).IsFocusable(false).Visibility(EVisibility::HitTestInvisible)
                        .IsChecked(Item->bIllicit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)];

            FText Text;
            if (Column == "Name") Text = FText::FromString(Item->DisplayName);
            else if (Column == "Type") Text = StaticEnum<ECargoType>()->GetDisplayNameTextByValue(int64(Item->Type));
            else if (Column == "Size")
                Text = Item->Size > 0 ? FText::Format(INVTEXT("{0} t"), FText::AsNumber(Item->Size)) : INVTEXT("-");
            else if (Column == "Quantity") Text = FText::AsNumber(Item->Quantity);
            else if (Column == "Rarity") Text = StaticEnum<ESSWAsteroidRarity>()->GetDisplayNameTextByValue(int64(Item->Rarity));
            else if (Column == "Unit") Text = FText::FromString(Item->Unit);
            else
            {
                const double Credits = Column == "Value" ? Item->Value : Item->TotalValue;
                Text = Credits >= 0 ? FText::Format(INVTEXT("{0} cr"), FText::AsNumber(Credits)) : INVTEXT("-");
            }
            return SNew(STextBlock).Margin(FMargin(6, 4)).Font(MissionUIStyle::GetRowFont(12)).Text(Text.ToUpper());
        }

    private:
        FCargoItem Item;
    };

    struct FCargoTable
    {
        TArray<FCargoItem> Rows;
        TWeakPtr<SListView<FCargoItem>> List;
        TWeakObjectPtr<UCargoCatalogSubsystem> Catalog;
        FName SortColumn = "Name";
        EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
        const Ship* LastShip = nullptr;
        FString Compartment;
        FText ServiceMessage;

        void Sort()
        {
            Rows.StableSort([this](const FCargoItem& A, const FCargoItem& B)
            {
                if (SortMode == EColumnSortMode::Descending) return Less(*B, *A);
                return Less(*A, *B);
            });
            if (auto View = List.Pin()) View->RequestListRefresh();
        }

        bool Less(const FS_Cargo& A, const FS_Cargo& B) const
        {
            if (SortColumn == "Type") return A.Type < B.Type;
            if (SortColumn == "Size") return A.Size < B.Size;
            if (SortColumn == "Quantity") return A.Quantity < B.Quantity;
            if (SortColumn == "Rarity") return A.Rarity < B.Rarity;
            if (SortColumn == "Illicit") return A.bIllicit < B.bIllicit;
            if (SortColumn == "Unit") return A.Unit.Compare(B.Unit, ESearchCase::IgnoreCase) < 0;
            if (SortColumn == "Value") return A.Value < B.Value;
            if (SortColumn == "TotalValue") return A.TotalValue < B.TotalValue;
            return A.DisplayName.Compare(B.DisplayName, ESearchCase::IgnoreCase) < 0;
        }

        void Refresh()
        {
            const Ship* Pilot = CargoShip();
            TArray<FS_Cargo> Current;
            if (auto* Craft = CargoShip())
            {
                Craft->SyncCargoInventory();
                const auto& State = Sim::GetSim()->ReadEconomy();
                const FName Key = Compartment.IsEmpty() ? Craft->GetCargoInventoryKey() : FStationServices::Inventory(*Compartment);
                if (const auto* Inventory = State.Inventories.Find(Key))
                    for (const auto& Stack : Inventory->Items)
                    {
                        if (Stack.Quantity <= 0) continue;
                        FS_Cargo Row; Row.Name = Stack.Type.ToString(); Row.DisplayName = Row.Name;
                        Row.Size = Stack.UnitTonnes; Row.Quantity = Stack.Quantity;
                        if (Catalog.IsValid()) Catalog->ForEachDefinition([&](FName Name, const FS_CargoCatalogRow& Definition)
                        {
                            if (Name != Stack.Type) return;
                            Row.DisplayName = Definition.DisplayName; Row.Type = Definition.Type; Row.Unit = Definition.Unit;
                            Row.Value = Definition.Value; Row.Rarity = Definition.Rarity; Row.bIllicit = Definition.bIllicit; Row.Image = Definition.Image;
                        });
                        if (Row.Name.StartsWith(TEXT("Courier/"))) { Row.DisplayName = TEXT("Sealed Courier Package"); Row.Unit = TEXT("crate"); }
                        Row.UpdateTotalValue(); Current.Add(MoveTemp(Row));
                    }
            }

            // Passenger berths remain separate from mass-based inventories.
            if (Pilot && Catalog.IsValid())
                if (const auto* Load = Pilot->GetCargoManifest().GetPtrOrNull(); Load && Load->bPassengers)
                {
                    FS_Cargo Passengers;
                    if (Catalog->DescribeCargo(*Pilot, Passengers))
                    { Passengers.Image = TEXT("Passengers"); Current.Add(MoveTemp(Passengers)); }
                }

            if (LastShip == Pilot && Current.Num() == Rows.Num() &&
                Algo::AllOf(Current, [this](const FS_Cargo& Item)
                {
                    return Rows.ContainsByPredicate([&Item](const FCargoItem& Row)
                    {
                        return Item.Name == Row->Name && Item.DisplayName == Row->DisplayName &&
                            Item.Unit == Row->Unit && Item.Rarity == Row->Rarity && Item.Type == Row->Type &&
                            Item.Size == Row->Size && Item.Quantity == Row->Quantity &&
                            Item.Value == Row->Value && Item.bIllicit == Row->bIllicit;
                    });
                }))
                return;

            auto View = List.Pin();
            const auto Previous = View && LastShip == Pilot ? View->GetSelectedItems() : TArray<FCargoItem>{};
            if (View) View->ClearSelection();
            LastShip = Pilot;
            Rows.Reset();
            for (auto& Item : Current)
                Rows.Add(MakeShared<FS_Cargo>(MoveTemp(Item)));
            Sort();
            if (!Previous.IsEmpty())
                if (const auto* Match = Rows.FindByPredicate([&Previous](const FCargoItem& Item)
                    { return Item->Name == Previous[0]->Name; }))
                    View->SetSelection(*Match);
        }
    };

}

TSharedRef<SWidget> UCargoManifestDlg::RebuildWidget()
{
    const auto Table = MakeShared<FCargoTable>();
    if (UGameInstance* Instance = GetGameInstance())
        Table->Catalog = Instance->GetSubsystem<UCargoCatalogSubsystem>();
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
    AddColumn("Name", INVTEXT("Name"), 2.f);
    AddColumn("Type", INVTEXT("Type"), 1.4f);
    AddColumn("Size", INVTEXT("Size"), 1.f);
    AddColumn("Quantity", INVTEXT("Quantity"), 1.f);
    AddColumn("Rarity", INVTEXT("RARITY"), 1.1f);
    AddColumn("Illicit", INVTEXT("ILLICIT"), 0.8f);
    AddColumn("Unit", INVTEXT("Unit"), 1.f);
    AddColumn("Value", INVTEXT("Unit value"), 1.f);
    AddColumn("TotalValue", INVTEXT("Total value"), 1.f);
    auto List = SNew(SListView<FCargoItem>).ListItemsSource(&Table->Rows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([Table](FCargoItem Item, const TSharedRef<STableViewBase>& Owner)
            -> TSharedRef<ITableRow>
        {
            return SNew(SCargoRow, Owner).Item(Item);
        });
    Table->List = List;
    Table->Refresh();
    List->RegisterActiveTimer(0.25f, FWidgetActiveTimerDelegate::CreateLambda([Table](double, float)
    {
        Table->Refresh();
        return EActiveTimerReturnType::Continue;
    }));
    const auto Font = MissionUIStyle::GetRowFont(13);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(20, 18, 20, 8)
        [
            SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(20))
            .Text_Lambda([]()
            {
                const Ship* Pilot = CargoShip();
                return Pilot ? FText::FromString(Pilot->GetContactLabel()).ToUpper() : INVTEXT("NO CRAFT AVAILABLE");
            })
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(20, 0, 20, 18)
        [
            SNew(STextBlock).Font(Font)
            .Text_Lambda([]()
            {
                const Ship* Pilot = CargoShip();
                if (!Pilot || !Pilot->Design()) return FText::GetEmpty();
                const auto& Capacity = Pilot->Design()->CargoCapacity;
                const FText Mass = FText::Format(INVTEXT("CARGO  {0} / {1} t"), FText::AsNumber(Pilot->GetCargoUsedTonnes()),
                    FText::AsNumber(Capacity.Tonnes));
                return Capacity.PassengerBerths > 0
                    ? FText::Format(INVTEXT("{0}    PASSENGER BERTHS  {1}"), Mass, FText::AsNumber(Capacity.PassengerBerths))
                    : Mass;
            })
        ]
        + SVerticalBox::Slot().FillHeight(1).Padding(20, 0)
        [MissionUIStyle::MakeDataPane(List, FMargin(0))]
        + SVerticalBox::Slot().AutoHeight().Padding(20, 4)
        [
            SNew(STextBlock).Font(Font)
            .Visibility_Lambda([Table]() { return Table->Rows.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
            .Text(INVTEXT("CARGO HOLD EMPTY"))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(20,8)
        [SNew(STextBlock).Font(Font).Text_Lambda([Table]() { return Table->ServiceMessage; })]
        + SVerticalBox::Slot().AutoHeight().Padding(20)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Ship cargo"))
                .OnClicked_Lambda([Table]() { Table->Compartment.Empty(); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Cargo)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Local station storage")) .IsEnabled_Lambda([](){return FStationServices::IsDocked();})
                .OnClicked_Lambda([Table]() { Table->Compartment = TEXT("Player Storage"); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Hangar)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Facility input inventory")) .IsEnabled_Lambda([](){return FStationServices::HasIndustry();})
                .OnClicked_Lambda([Table]() { Table->Compartment = TEXT("Player Input"); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Production)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Collect completed facility output")) .IsEnabled_Lambda([](){return FStationServices::HasIndustry();})
                .OnClicked_Lambda([Table]() { Table->Compartment = TEXT("Player Output"); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(8,0)
            [SNew(STextBlock).Font(Font).Text_Lambda([Table]() { return FText::FromString(Table->Compartment.IsEmpty() ? TEXT("SHIP CARGO") : Table->Compartment.ToUpper()); })]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Transfer selected cargo: ship to local storage, or selected station inventory to ship"))
                .IsEnabled_Lambda([](){return FStationServices::IsDocked();})
                .OnClicked_Lambda([Table, WeakInstance = TWeakObjectPtr<UGameInstance>(GetGameInstance())]()
                {
                    if (auto View = Table->List.Pin(); View && !View->GetSelectedItems().IsEmpty())
                        FStationServices::MoveCargo(WeakInstance.Get(), FName(*View->GetSelectedItems()[0]->Name), Table->Compartment.IsEmpty(),
                            Table->Compartment.IsEmpty() ? TEXT("Player Storage") : *Table->Compartment, Table->ServiceMessage);
                    Table->Refresh(); return FReply::Handled();
                })[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Replace)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText(INVTEXT("Supply selected ship cargo to this facility input inventory"))
                .IsEnabled_Lambda([Table](){return Table->Compartment.IsEmpty() && FStationServices::HasIndustry();})
                .OnClicked_Lambda([Table, WeakInstance = TWeakObjectPtr<UGameInstance>(GetGameInstance())]()
                {
                    if (auto View = Table->List.Pin(); View && !View->GetSelectedItems().IsEmpty())
                        FStationServices::MoveCargo(WeakInstance.Get(), FName(*View->GetSelectedItems()[0]->Name), true, TEXT("Player Input"), Table->ServiceMessage);
                    Table->Refresh(); return FReply::Handled();
                })[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Production)]]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [SNew(SButton).ToolTipText_Lambda([](){return FText::Format(INVTEXT("Full repair: {0} credits. Restores hull and installed components while docked."), FText::AsNumber(FStationServices::FullRepairCost()));})
                .IsEnabled_Lambda([](){return FStationServices::FullRepairCost() > 0;})
                .OnClicked_Lambda([Table, WeakInstance = TWeakObjectPtr<UGameInstance>(GetGameInstance())]()
                { FStationServices::RepairAll(WeakInstance.Get(), Table->ServiceMessage); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Repair)]]
        ];
}
