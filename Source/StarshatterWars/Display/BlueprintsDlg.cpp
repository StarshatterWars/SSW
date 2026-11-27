#include "BlueprintsDlg.h"
#include "RecordDetails.h"
#include "ModifierProvenance.h"
#include "ProductionCatalog.h"
#include "BlueprintCatalogSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Sim.h"
#include "StationServices.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
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
    FText Feedback, Preflight;
    FPlayerIndustryQuote Quote;
    FGuid License;
    int32 Runs = 1;
    bool CanInstall = false;
    double LastPreflight = 0;

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
void RefreshPreflight(const TWeakObjectPtr<UBlueprintCatalogSubsystem>& Catalog, const TSharedPtr<FBlueprintTable>& Model)
{
    const double Now = FPlatformTime::Seconds();
    if (Model->LastPreflight > 0 && Now - Model->LastPreflight < .5) return;
    Model->LastPreflight = Now; Model->CanInstall = false; Model->License = {}; Model->Quote = {};
    const auto* Site = FStationServices::Facility(); const auto* Simulation = Sim::GetSim();
    if (!Catalog.IsValid() || !Model->Selected || !Site || !Simulation)
    { Model->Preflight = INVTEXT("Select a blueprint and dock at its production facility to install a job."); return; }
    if (!Site->SupportsRecipe(Model->Selected->Recipe))
    { Model->Preflight = INVTEXT("This facility does not manufacture the selected recipe."); return; }
    const FBlueprintInstance* Selected = nullptr;
    for (const auto& Pair : Simulation->ReadEconomy().Blueprints)
    {
        const auto& BP = Pair.Value;
        if (BP.Owner != FEconomyLedger::Pilot() || BP.Recipe != Model->Selected->Recipe || (!BP.bOriginal && BP.RunsRemaining < Model->Runs) ||
            Simulation->ReadEconomy().Jobs.ContainsByPredicate([&](const auto& Job) { return Job.Blueprint == Pair.Key &&
                Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled; })) continue;
        if (!Selected || (BP.bOriginal && !Selected->bOriginal) || (BP.bOriginal == Selected->bOriginal &&
            (BP.RunsRemaining < Selected->RunsRemaining || (BP.RunsRemaining == Selected->RunsRemaining && BP.Id.ToString() < Selected->Id.ToString()))))
            Selected = &BP;
    }
    if (!Selected) { Model->Preflight = INVTEXT("Purchase an available original or copy with enough licensed runs first."); return; }
    Model->License = Selected->Id;
    Model->CanInstall = Catalog->CanBeginPlayerJob(*Site, Model->License, Model->Runs, Model->Quote, Model->Preflight);
}
FRecordDetails DescribeQuote(const TSharedPtr<FBlueprintTable>& Model)
{
    FRecordDetails D;D.Icon=EShipSystemIcon::Production;
    D.Title=Model->Selected?FText::FromString(Model->Selected->DisplayName):INVTEXT("SELECT A BLUEPRINT");
    D.Status=Model->CanInstall?INVTEXT("READY TO INSTALL"):INVTEXT("REQUIREMENTS");D.Description=Model->Preflight;
    if(!Model->Quote.Materials.IsEmpty()){
        D.Fact(INVTEXT("BATCH"),FText::Format(INVTEXT("{0} RUNS"),FText::AsNumber(Model->Runs)));
        D.Fact(INVTEXT("TOTAL TIME"),FText::Format(INVTEXT("{0} S"),FText::AsNumber(Model->Quote.Duration)));
        D.Fact(INVTEXT("INPUT INVENTORY"),FText::FromName(Model->Quote.Input)).Fact(INVTEXT("OUTPUT INVENTORY"),FText::FromName(Model->Quote.Output));
        D.Fact(INVTEXT("SERVICE"),FText::FromName(Model->Quote.ServiceProfile));
        D.Fact(INVTEXT("INSTALLATION"),FText::Format(INVTEXT("{0} CR"),FText::AsNumber(Model->Quote.InstallationFee)));
        TArray<FText> Inputs,Outputs;
        for(const auto& A:Model->Quote.Materials)Inputs.Add(FText::Format(INVTEXT("{0} / QTY {1} / {2} T"),FText::FromName(A.Item),FText::AsNumber(A.Quantity),FText::AsNumber(A.Quantity*A.UnitTonnes)));
        for(const auto& A:Model->Quote.Products)Outputs.Add(FText::Format(INVTEXT("{0} / QTY {1} / {2} T"),FText::FromName(A.Item),FText::AsNumber(A.Quantity),FText::AsNumber(A.Quantity*A.UnitTonnes)));
        D.Section(INVTEXT("REQUIRED MATERIALS"),FText::Join(INVTEXT("\n"),Inputs));D.Section(INVTEXT("FINISHED PRODUCTS"),FText::Join(INVTEXT("\n"),Outputs));
    }
    if(const auto* Site=FStationServices::Facility();Site && Model->Selected)
        if(const auto* Recipe=SSWProductionCatalog::Recipes().Find(Model->Selected->Recipe))
        {const auto* S=Sim::GetSim();const auto* BP=S?S->ReadEconomy().Blueprints.Find(Model->License):nullptr;ModifierProvenance::Industry(D,*Recipe,*Site,BP,Model->CanInstall?&Model->Quote:nullptr);}
    return D;
}
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
        .OnSelectionChanged_Lambda([Model](FOffer Item,ESelectInfo::Type) { Model->Selected=Item; Model->Feedback=FText::GetEmpty(); Model->LastPreflight=0; });
    Model->List=List;
    auto Buttons=SNew(SHorizontalBox);
    for (bool Original : {true,false})
        Buttons->AddSlot().AutoWidth().Padding(6,0)
        [SNew(SButton).ToolTipText(Original ? INVTEXT("Buy blueprint original from NPC publisher — unlimited runs") : INVTEXT("Buy limited-run blueprint copy from NPC publisher"))
            .IsEnabled_Lambda([Weak,Model,Original]() { FText Reason; return Weak.IsValid() && Model->Selected && Weak->CanPurchase(Model->Selected->Recipe,Original,Reason); })
            .OnClicked_Lambda([Weak,Model,Original]() { if (Weak.IsValid() && Model->Selected) Weak->RequestPurchase(Model->Selected->Recipe,Original,[WeakModel=TWeakPtr<FBlueprintTable>(Model)](bool,const FText& Why,FGuid){if(auto M=WeakModel.Pin()){M->Feedback=Why;M->Refresh();}},Model->Feedback); Model->Refresh(); return FReply::Handled(); })
            [SNew(SShipSystemIcon).Icon(Original ? EShipSystemIcon::Reference : EShipSystemIcon::Replace)]];
    Buttons->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(INVTEXT("RUNS"))];
    Buttons->AddSlot().AutoWidth().Padding(4,0)
        [SNew(SSpinBox<int32>).MinValue(1).MaxValue(1000000).MinDesiredWidth(96).Font(MissionUIStyle::GetRowFont(12))
            .Value_Lambda([Model]() { return Model->Runs; })
            .OnValueChanged_Lambda([Model](int32 Value) { Model->Runs=Value; Model->LastPreflight=0; })];
    Buttons->AddSlot().AutoWidth().Padding(6,0)
    [SNew(SButton).ToolTipText_Lambda([Weak,Model]() { RefreshPreflight(Weak,Model); return Model->Preflight; })
        .IsEnabled_Lambda([Weak,Model]() { RefreshPreflight(Weak,Model); return Model->CanInstall; })
        .OnClicked_Lambda([Weak,Model]()
        {
            Model->LastPreflight=0; RefreshPreflight(Weak,Model);
            const auto* Site=FStationServices::Facility();
            if (Weak.IsValid() && Site && Model->CanInstall) Weak->RequestBeginPlayerJob(*Site,Model->License,Model->Runs,[WeakModel=TWeakPtr<FBlueprintTable>(Model)](bool,const FText& Why,FGuid){if(auto M=WeakModel.Pin()){M->Feedback=Why;M->LastPreflight=0;M->Refresh();}},Model->Feedback);
            Model->LastPreflight=0; Model->Refresh(); return FReply::Handled();
        })[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Production)]];
    for(FName Activity:{FName(TEXT("ResearchMaterial")),FName(TEXT("ResearchTime")),FName(TEXT("Copying"))})
    {
        const auto Terms=[Weak,Model,Activity](FText& Reason)->bool
        {RefreshPreflight(Weak,Model);const auto* Site=FStationServices::Facility();double Seconds=0;int64 Price=0;return Weak.IsValid() && Site && Weak->CanBlueprintService(*Site,Model->License,Activity,Model->Runs,Seconds,Price,Reason);};
        Buttons->AddSlot().AutoWidth().Padding(6,0)[SNew(SButton)
            .ToolTipText_Lambda([Terms,Activity](){FText Why;Terms(Why);return FText::Format(INVTEXT("{0}\n{1}"),Activity==TEXT("ResearchMaterial")?INVTEXT("RESEARCH MATERIAL EFFICIENCY +1 PERCENTAGE POINT"):Activity==TEXT("ResearchTime")?INVTEXT("RESEARCH TIME EFFICIENCY +2 PERCENTAGE POINTS"):INVTEXT("MAKE A LICENSED COPY USING THE RUNS VALUE"),Why);})
            .IsEnabled_Lambda([Terms](){FText Why;return Terms(Why);})
            .OnClicked_Lambda([Weak,Model,Activity](){RefreshPreflight(Weak,Model);const auto* Site=FStationServices::Facility();if(Weak.IsValid() && Site)Weak->RequestBlueprintService(*Site,Model->License,Activity,Model->Runs,[W=TWeakPtr<FBlueprintTable>(Model)](bool,const FText& Why,FGuid){if(auto M=W.Pin()){M->Feedback=Why;M->LastPreflight=0;M->Refresh();}},Model->Feedback);return FReply::Handled();})
            [SNew(SShipSystemIcon).Icon(Activity==TEXT("Copying")?EShipSystemIcon::Replace:Activity==TEXT("ResearchMaterial")?EShipSystemIcon::Skills:EShipSystemIcon::Reactor)]];
    }
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SSearchBox).HintText(INVTEXT("Search NPC blueprints"))
            .OnTextChanged_Lambda([Model](const FText& Search) { Model->Search=Search.ToString().TrimStartAndEnd(); Model->Refresh(); })]
        + SVerticalBox::Slot().FillHeight(1).Padding(8)[MissionUIStyle::MakeDataPane(List)]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [MissionUIStyle::MakeDataPane(SNew(SBox).MaxDesiredHeight(170)
            [SNew(SScrollBox)+SScrollBox::Slot()
                [SNew(SRecordDetails).EventDriven(true).SelectionRevision_Lambda([Model](){return HashCombine(PointerHash(Model->Selected.Get()),GetTypeHash(Model->Runs));}).Record_Lambda([Weak,Model]()
                    { RefreshPreflight(Weak,Model); return DescribeQuote(Model); })]])]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Model]() { return Model->Feedback; })]
            + SHorizontalBox::Slot().AutoWidth()[Buttons]];
}
