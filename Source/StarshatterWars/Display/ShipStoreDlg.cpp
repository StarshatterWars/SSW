#include "ShipStoreDlg.h"
#include "SkillsSubsystem.h"
#include "ShipInsuranceSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameStructs.h"
#include "MissionUIStyle.h"
#include "PlayerCharacter.h"
#include "Ship.h"
#include "Sim.h"
#include "StarshatterShipDesignSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
struct FStoreEntry
{
    FString Name;
    FName Design;
    FText Category;
    FText Grade;
    FText Access;
    FText Empire;
    int32 RequiredRank = 0;
    int64 Credits = 0;
};

using FStoreRow = TSharedPtr<FStoreEntry>;
using FStoreRows = TArray<FStoreRow>;

class SStoreRow : public SMultiColumnTableRow<FStoreRow>
{
public:
    SLATE_BEGIN_ARGS(SStoreRow) {}
        SLATE_ARGUMENT(FStoreRow, Entry)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    {
        Entry = Args._Entry;
        SMultiColumnTableRow<FStoreRow>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner);
    }

    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        FText Label;
        if (Column == "Item") Label = FText::FromString(Entry->Name);
        else if (Column == "Category") Label = Entry->Category;
        else if (Column == "Grade") Label = Entry->Grade;
        else if (Column == "Access") Label = Entry->Access;
        else if (Column == "Empire") Label = Entry->Empire;
        else if (Column == "Price") Label = FText::AsNumber(Entry->Credits);

        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(8, 4))
            .Text(Label.ToUpper()).ToolTipText(Label.ToUpper()).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
            .Justification(Column == "Price" ? ETextJustify::Right : ETextJustify::Left);
    }

private:
    FStoreRow Entry;
};

TSharedRef<SWidget> MakeCatalog(const TSharedRef<FStoreRows>& Rows, bool bComponents,const TSharedRef<FName>& Selected)
{
    if (Rows->IsEmpty())
        return SNew(STextBlock).Margin(FMargin(8)).Text(INVTEXT("No catalog entries available."));

    struct FSortState
    {
        FName Column = "Item";
        EColumnSortMode::Type Mode = EColumnSortMode::Ascending;
        TWeakPtr<SListView<FStoreRow>> List;
    };
    auto SortState = MakeShared<FSortState>();
    const auto Sort = [Rows, SortState]()
    {
        Rows->StableSort([&SortState](const FStoreRow& Left, const FStoreRow& Right)
        {
            const FStoreEntry& A = *(SortState->Mode == EColumnSortMode::Descending ? Right : Left);
            const FStoreEntry& B = *(SortState->Mode == EColumnSortMode::Descending ? Left : Right);
            const FName Column = SortState->Column;
            if (Column == "Price" && A.Credits != B.Credits) return A.Credits < B.Credits;
            if (Column == "Access" && A.RequiredRank != B.RequiredRank) return A.RequiredRank < B.RequiredRank;
            if (Column == "Grade" && A.RequiredRank != B.RequiredRank) return A.RequiredRank < B.RequiredRank;
            if (Column == "Category")
            {
                const int32 Order = A.Category.ToString().Compare(B.Category.ToString(), ESearchCase::IgnoreCase);
                if (Order != 0) return Order < 0;
            }
            if (Column == "Empire")
            {
                const int32 Order = A.Empire.ToString().Compare(B.Empire.ToString(), ESearchCase::IgnoreCase);
                if (Order != 0) return Order < 0;
            }
            return A.Name.Compare(B.Name, ESearchCase::IgnoreCase) < 0;
        });
        if (auto List = SortState->List.Pin()) List->RequestListRefresh();
    };

    auto Header = SNew(SHeaderRow);
    const auto AddColumn = [&Header, SortState, Sort](FName Column, FText Label, float Width)
    {
        Header->AddColumn(SHeaderRow::Column(Column).DefaultLabel(Label).FillWidth(Width)
            .SortMode_Lambda([SortState, Column]()
            {
                return SortState->Column == Column ? SortState->Mode : EColumnSortMode::None;
            })
            .OnSort_Lambda([SortState, Sort](EColumnSortPriority::Type, const FName& Id, EColumnSortMode::Type Mode)
            {
                SortState->Column = Id;
                SortState->Mode = Mode;
                Sort();
            }));
    };
    AddColumn("Item", bComponents ? INVTEXT("COMPONENT") : INVTEXT("HULL"), 2.f);
    AddColumn("Category", bComponents ? INVTEXT("SYSTEM") : INVTEXT("CLASS"), 1.3f);
    if (bComponents)
        AddColumn("Grade", INVTEXT("GRADE"), 0.6f);
    else
    {
        AddColumn("Empire", INVTEXT("EMPIRE"), 1.4f);
        AddColumn("Access", INVTEXT("RANK / SKILL"), 1.2f);
    }
    AddColumn("Price", INVTEXT("BASE COST (CR)"), 1.1f);

    Sort();
    auto List = SNew(SListView<FStoreRow>).ListItemsSource(&*Rows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::Single)
        .OnSelectionChanged_Lambda([Selected,bComponents](FStoreRow Row,ESelectInfo::Type){*Selected=Row && !bComponents?Row->Design:NAME_None;})
        .OnGenerateRow_Lambda([Rows](FStoreRow Entry, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
        {
            return SNew(SStoreRow, Owner).Entry(Entry);
        });
    SortState->List = List;
    return List;
}
}

TSharedRef<SWidget> UShipStoreDlg::RebuildWidget()
{
    auto Hulls = MakeShared<FStoreRows>();
    auto Components = MakeShared<FStoreRows>();
    auto* Game = GetGameInstance();
    FText ReserveReason;
    if(auto* Sales=Game?Game->GetSubsystem<UShipInsuranceSubsystem>():nullptr)Sales->PrepareCarrierFighterSales(ReserveReason);
    const auto* ShipCatalog = Game ? Game->GetSubsystem<UStarshatterShipDesignSubsystem>() : nullptr;
    const auto* SystemCatalog = Game ? Game->GetSubsystem<UStarshatterSystemDesignSubsystem>() : nullptr;
    if (ShipCatalog)
    {
        const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
        const int32 Rank = Pilot ? Pilot->GetRank() : 0;
        const auto* EmpireEnum = StaticEnum<EShipEmpire>();
        const int64 PilotEmpire = Pilot ? EmpireEnum->GetValueByNameString(
            StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(Pilot->GetEmpire()))) : INDEX_NONE;
        for (const auto& Entry : ShipCatalog->GetDesignsByName())
        {
            const FShipDesign* Hull = &Entry.Value;
            if (Hull->Hidden || (Hull->Category != EShipCategory::CapitalShip &&
                Hull->Category != EShipCategory::Fighter && Hull->Category != EShipCategory::Shuttle &&
                Hull->Category != EShipCategory::Transport))
                continue;

            const int32 RequiredRank = PlayerCharacter::HullRankRequired(Hull->ShipType,Hull->ShipName);
            if (Rank < RequiredRank || Hull->BaseCost <= 0 ||
                (Hull->ShipEmpire != EShipEmpire::All && Hull->ShipEmpire != EShipEmpire::Neutral && static_cast<int64>(Hull->ShipEmpire) != PilotEmpire))
                continue;

            auto Row = MakeShared<FStoreEntry>();
            Row->Design=FName(*Hull->ShipName);
            Row->Name = Hull->DisplayName.IsEmpty() ? Hull->ShipName : Hull->DisplayName;
            Row->Category = FText::FromString(Hull->ShipClass);
            Row->Empire = EmpireEnum->GetDisplayNameTextByValue(static_cast<int64>(Hull->ShipEmpire));
            Row->RequiredRank = RequiredRank;
            Row->Access = FText::FromString(ANSI_TO_TCHAR(PlayerCharacter::RankName(RequiredRank)));
            const FName RequiredSkill=USkillsSubsystem::RequiredHullSkill(Hull->ShipType);
            if (!RequiredSkill.IsNone())
            {
                const auto* Skills=GetGameInstance()->GetSubsystem<USkillsSubsystem>();
                const auto* Definition=Skills ? Skills->GetCatalog().Find(RequiredSkill) : nullptr;
                Row->Access=FText::Format(INVTEXT("{0} + {1} ({2})"),Row->Access,
                    Definition ? FText::FromString(Definition->DisplayName) : FText::FromName(RequiredSkill),
                    Skills && Skills->HasSkill(RequiredSkill) ? INVTEXT("COMPLETE") : INVTEXT("REQUIRED"));
            }
            const auto* Sales=Game->GetSubsystem<UShipInsuranceSubsystem>();
            Row->Credits = Sales?Sales->HullPurchasePrice(Row->Design):Hull->BaseCost;
            Hulls->Add(Row);
        }

    }

    if (SystemCatalog)
    {
        static const TCHAR* GradeNames[] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V") };
        for (const auto& Entry : SystemCatalog->GetDesignsByName())
        {
            const FSystemDesign* System = &Entry.Value;
            for (const auto& Part : System->Components)
            {
                for (const auto& Grade : System->ComponentGrades)
                {
                    if (Grade.Level < 1 || Grade.Level > UE_ARRAY_COUNT(GradeNames))
                        continue;

                    auto Row = MakeShared<FStoreEntry>();
                    Row->Name = Part.Name;
                    Row->Category = FText::FromString(System->Name);
                    Row->RequiredRank = Grade.Level;
                    Row->Grade = FText::FromString(GradeNames[Grade.Level - 1]);
                    const auto Variant = SystemCatalog->FindComponentGrade(Entry.Key, FName(*Part.Name), Grade.Level);
                    Row->Credits = Variant ? Variant->BaseCost : 0;
                    Components->Add(Row);
                }
            }
        }
    }

    Sim* Simulation = Sim::GetSim();
    const Ship* ActiveShip = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const FText CurrentHull = ActiveShip
        ? FText::Format(INVTEXT("Current ship: {0}"), FText::FromString(ANSI_TO_TCHAR(ActiveShip->GetObserverName())))
        : INVTEXT("No active ship");

    const auto Selected=MakeShared<FName>();const auto Feedback=MakeShared<FText>(ReserveReason);
    auto Tabs = SNew(SWidgetSwitcher)
        + SWidgetSwitcher::Slot()[MakeCatalog(Hulls, false,Selected)]
        + SWidgetSwitcher::Slot()[MakeCatalog(Components, true,Selected)];

    const auto SelectTab = [Switcher = TWeakPtr<SWidgetSwitcher>(Tabs)](int32 Index)
    {
        if (const auto Widget = Switcher.Pin())
            Widget->SetActiveWidgetIndex(Index);
        return FReply::Handled();
    };

    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(CurrentHull)]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
        [SNew(STextBlock).Text(INVTEXT("Carrier aircraft availability follows the OOB squadrons. Training Stormhawk price: 400,000 CR. Completed ships and refits are available at shipyards."))]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
            [SNew(SButton).Text(INVTEXT("Hulls")).OnClicked_Lambda([SelectTab]() { return SelectTab(0); })]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).Text(INVTEXT("Components")).OnClicked_Lambda([SelectTab]() { return SelectTab(1); })]
        ]
        + SVerticalBox::Slot().FillHeight(1.f).Padding(8, 4)[MissionUIStyle::MakeDataPane(Tabs, FMargin(0))]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            [SNew(SButton).Text(INVTEXT("PURCHASE HULL"))
                .IsEnabled_Lambda([this,Selected,WeakTabs=TWeakPtr<SWidgetSwitcher>(Tabs)](){FText Why;const auto T=WeakTabs.Pin();const auto* Service=GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>();return T && T->GetActiveWidgetIndex()==0 && Service && Service->CanPurchaseHull(*Selected,Why);})
                .ToolTipText_Lambda([this,Selected](){FText Why;if(const auto* Service=GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>())Service->CanPurchaseHull(*Selected,Why);return Why;})
                .OnClicked_Lambda([this,Selected,Feedback](){if(auto* Service=GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>())Service->PurchaseHull(*Selected,*Feedback);return FReply::Handled();})]
            + SHorizontalBox::Slot().Padding(6, 0)
            [SNew(SButton).IsEnabled(false).Text(INVTEXT("Refit Current Ship"))]
            + SHorizontalBox::Slot()
            [SNew(SButton).IsEnabled(false).Text(INVTEXT("Insure Current Ship"))]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(8,4)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Feedback](){return *Feedback;})]
        + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
        [
            MissionUIStyle::MakeDataPane(
                SNew(STextBlock).AutoWrapText(true)
                .Text(INVTEXT("Full coverage includes the hull and all installed components.\nCash-purchased ships include lifetime full coverage.")))
        ];
}
