#include "BlueprintsDlg.h"
#include "BlueprintCatalogSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Sim.h"
#include "StationServices.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
namespace
{
using FOffer = TSharedPtr<FBlueprintOffer>;
FString Owned(FName Recipe)
{
    const auto* Simulation = Sim::GetSim(); if (!Simulation) return TEXT("NONE");
    int32 Originals = 0, Copies = 0, Runs = 0;
    for (const auto& Pair : Simulation->ReadEconomy().Blueprints)
        if (Pair.Value.Owner == FEconomyLedger::Pilot() && Pair.Value.Recipe == Recipe)
        { if (Pair.Value.bOriginal) ++Originals; else { ++Copies; Runs += Pair.Value.RunsRemaining; } }
    return FString::Printf(TEXT("%d ORIGINAL / %d COPY / %d RUNS"), Originals, Copies, Runs);
}
class SBlueprintRow : public SMultiColumnTableRow<FOffer>
{
public:
    SLATE_BEGIN_ARGS(SBlueprintRow) {} SLATE_ARGUMENT(FOffer, Item) SLATE_END_ARGS()
    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    { Item = Args._Item; SMultiColumnTableRow<FOffer>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner); }
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        if (Column == TEXT("Name"))
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4,2)[SNew(SShipSystemIcon).Icon(SShipSystemIcon::ForCatalogImage(Item->Image))]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Item->DisplayName).ToUpper())];
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(6,4)).Text_Lambda([Item=Item,Column]()
        {
            if (Column == TEXT("Original")) return FText::AsNumber(Item->OriginalPrice);
            if (Column == TEXT("Copy")) return FText::Format(INVTEXT("{0} CR / {1} RUNS"), FText::AsNumber(Item->CopyPrice), FText::AsNumber(Item->CopyRuns));
            if (Column == TEXT("Owned")) return FText::FromString(Owned(Item->Recipe));
            return FText::FromString(Item->DisplayName).ToUpper();
        });
    }
private: FOffer Item;
};
struct FBlueprintTable
{
    TArray<FOffer> All, Visible;
    TWeakPtr<SListView<FOffer>> List;
    FOffer Selected;
    FString Search;
    FName Column = TEXT("Name"); bool Descending = false;
    FText Feedback;
    void Refresh()
    {
        Visible.Reset(); for (const auto& Row : All) if (Search.IsEmpty() || Search == TEXT("*") || Row->DisplayName.Contains(Search)) Visible.Add(Row);
        Visible.StableSort([this](const auto& A, const auto& B)
        {
            int32 Result = 0;
            if (Column == TEXT("Original")) Result = A->OriginalPrice < B->OriginalPrice ? -1 : A->OriginalPrice > B->OriginalPrice ? 1 : 0;
            else if (Column == TEXT("Copy")) Result = A->CopyPrice < B->CopyPrice ? -1 : A->CopyPrice > B->CopyPrice ? 1 : 0;
            else if (Column == TEXT("Owned")) Result = Owned(A->Recipe).Compare(Owned(B->Recipe));
            if (!Result) Result = A->DisplayName.Compare(B->DisplayName, ESearchCase::IgnoreCase);
            return Descending ? Result > 0 : Result < 0;
        });
        if (auto View = List.Pin()) View->RequestListRefresh();
    }
};
}
TSharedRef<SWidget> UBlueprintsDlg::RebuildWidget()
{
    auto* Catalog = GetGameInstance()->GetSubsystem<UBlueprintCatalogSubsystem>();
    auto Model = MakeShared<FBlueprintTable>(); const TWeakObjectPtr<UBlueprintCatalogSubsystem> Weak(Catalog);
    if (Catalog) for (const auto& Pair : Catalog->GetOffers()) Model->All.Add(MakeShared<FBlueprintOffer>(Pair.Value));
    Model->Refresh(); auto Header = SNew(SHeaderRow);
    for (const auto& Entry : {TPair<FName,FString>(TEXT("Name"),TEXT("BLUEPRINT")),{TEXT("Original"),TEXT("ORIGINAL / CR")},{TEXT("Copy"),TEXT("LIMITED COPY")},{TEXT("Owned"),TEXT("YOUR LIBRARY")}})
        Header->AddColumn(SHeaderRow::Column(Entry.Key).DefaultLabel(FText::FromString(Entry.Value)).FillWidth(Entry.Key == TEXT("Name") ? 2.f : 1.4f)
            .SortMode_Lambda([Model,Id=Entry.Key]() { return Model->Column != Id ? EColumnSortMode::None : Model->Descending ? EColumnSortMode::Descending : EColumnSortMode::Ascending; })
            .OnSort_Lambda([Model](EColumnSortPriority::Type,const FName& Column,EColumnSortMode::Type Mode) { Model->Column=Column; Model->Descending=Mode==EColumnSortMode::Descending; Model->Refresh(); }));
    auto List=SNew(SListView<FOffer>).ListItemsSource(&Model->Visible).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([](FOffer Item,const TSharedRef<STableViewBase>& Owner) { return SNew(SBlueprintRow,Owner).Item(Item); })
        .OnSelectionChanged_Lambda([Model](FOffer Item,ESelectInfo::Type) { Model->Selected=Item; Model->Feedback=FText::GetEmpty(); });
    Model->List=List;
    auto Buttons=SNew(SHorizontalBox);
    for (bool Original : {true,false})
        Buttons->AddSlot().AutoWidth().Padding(6,0)
        [SNew(SButton).ToolTipText(Original ? INVTEXT("Buy blueprint original from NPC publisher — unlimited runs") : INVTEXT("Buy limited-run blueprint copy from NPC publisher"))
            .IsEnabled_Lambda([Weak,Model,Original]() { FText Reason; return Weak.IsValid() && Model->Selected && Weak->CanPurchase(Model->Selected->Recipe,Original,Reason); })
            .OnClicked_Lambda([Weak,Model,Original]() { if (Weak.IsValid() && Model->Selected) Weak->Purchase(Model->Selected->Recipe,Original,Model->Feedback); Model->Refresh(); return FReply::Handled(); })
            [SNew(SShipSystemIcon).Icon(Original ? EShipSystemIcon::Reference : EShipSystemIcon::Replace)]];
    Buttons->AddSlot().AutoWidth().Padding(6,0)
    [SNew(SButton).ToolTipText(INVTEXT("Start one batch using an owned blueprint at this docked production facility"))
        .IsEnabled_Lambda([Model]() { const auto* Site = FStationServices::Facility(); return Site && Model->Selected && Site->Recipe == Model->Selected->Recipe; })
        .OnClicked_Lambda([Weak,Model]()
        {
            const auto* Site = FStationServices::Facility(); auto* Simulation = Sim::GetSim();
            if (!Weak.IsValid() || !Site || !Simulation || !Model->Selected || Site->Recipe != Model->Selected->Recipe) return FReply::Handled();
            FGuid License;
            for (const auto& Pair : Simulation->ReadEconomy().Blueprints)
                if (Pair.Value.Owner == FEconomyLedger::Pilot() && Pair.Value.Recipe == Site->Recipe &&
                    (Pair.Value.bOriginal || Pair.Value.RunsRemaining > 0) &&
                    !Simulation->ReadEconomy().Jobs.ContainsByPredicate([&](const auto& Job) { return Job.Blueprint == Pair.Key &&
                        Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled; }))
                { License = Pair.Key; break; }
            if (!License.IsValid()) Model->Feedback = INVTEXT("Purchase an available blueprint original or copy first.");
            else Weak->BeginPlayerJob(*Site, License, 1, Model->Feedback);
            Model->Refresh(); return FReply::Handled();
        })[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Production)]];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SSearchBox).HintText(INVTEXT("Search NPC blueprints"))
            .OnTextChanged_Lambda([Model](const FText& Search) { Model->Search=Search.ToString().TrimStartAndEnd(); Model->Refresh(); })]
        + SVerticalBox::Slot().FillHeight(1).Padding(8)[MissionUIStyle::MakeDataPane(List)]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Weak,Model]()
            { if (!Model->Selected) return INVTEXT("Select a blueprint. A purchased original or copy is required for player manufacturing. NPC facilities own separate licenses.");
              FText Reason; if (Weak.IsValid()) Weak->CanPurchase(Model->Selected->Recipe,true,Reason); return Reason; })]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Model]() { return Model->Feedback; })]
            + SHorizontalBox::Slot().AutoWidth()[Buttons]];
}
