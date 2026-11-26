#include "IndustryJobsPanel.h"
#include "ChangeDrivenPanel.h"

#include "BlueprintCatalogSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "StationServices.h"
#include "Sim.h"
#include "Ship.h"
#include "Engine/GameInstance.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
namespace
{
using FJobRow=TSharedPtr<FIndustryJob>;
bool Operational(const FIndustryJob& Job)
{
    auto* Simulation=Sim::GetSim();
    if(!Simulation)return false;
    const auto& Economy=Simulation->ReadEconomy();const auto* Input=Economy.Inventories.Find(Job.Input);const auto* Output=Economy.Inventories.Find(Job.Output);
    return Input && Output && Input->bOperational && Output->bOperational;
}
FText JobState(const FIndustryJob& Job)
{
    switch(Job.State)
    {
    case EIndustryJobState::Completed:return INVTEXT("COMPLETED");
    case EIndustryJobState::Cancelled:return INVTEXT("CANCELLED");
    case EIndustryJobState::Blocked:return INVTEXT("BLOCKED");
    case EIndustryJobState::Running:return !Operational(Job)?INVTEXT("PAUSED"):Job.Remaining<=0?INVTEXT("OUTPUT BLOCKED"):INVTEXT("RUNNING");
    default:return INVTEXT("QUEUED");
    }
}
class SIndustryJobRow : public SMultiColumnTableRow<FJobRow>
{
public:
    SLATE_BEGIN_ARGS(SIndustryJobRow){} SLATE_ARGUMENT(FJobRow,Item) SLATE_END_ARGS()
    void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
    { Item=Args._Item; SMultiColumnTableRow<FJobRow>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner); }
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        FText Text;
        if(Column==TEXT("Id"))Text=FText::FromString(Item->Id.ToString(EGuidFormats::Digits).Left(8));
        else if(Column==TEXT("Facility"))Text=FText::FromName(Item->Facility);
        else if(Column==TEXT("Recipe"))Text=FText::FromName(Item->Recipe);
        else if(Column==TEXT("Input"))Text=FText::FromName(Item->Input);
        else if(Column==TEXT("Output"))Text=FText::FromName(Item->Output);
        else if(Column==TEXT("Runs"))Text=FText::AsNumber(Item->Runs);
        else if(Column==TEXT("State"))Text=JobState(*Item);
        else if(Column==TEXT("Time"))Text=FText::Format(INVTEXT("{0} S"),FText::AsNumber(FMath::CeilToInt(Item->Remaining)));
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(6,4)).Text(Text.ToUpper()).ToolTipText(Text).OverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }
private:FJobRow Item;
};
class SPlayerIndustryJobs : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPlayerIndustryJobs){} SLATE_ARGUMENT(UGameInstance*,Instance) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Catalog=Args._Instance?Args._Instance->GetSubsystem<UBlueprintCatalogSubsystem>():nullptr;
        auto Header=SNew(SHeaderRow);
        for(const auto& Column:{TPair<FName,FString>(TEXT("Id"),TEXT("JOB")),{TEXT("Facility"),TEXT("FACILITY")},{TEXT("Recipe"),TEXT("RECIPE")},{TEXT("Runs"),TEXT("RUNS")},{TEXT("Input"),TEXT("INPUT")},{TEXT("Output"),TEXT("OUTPUT")},{TEXT("State"),TEXT("STATUS")},{TEXT("Time"),TEXT("REMAINING")}})
            Header->AddColumn(SHeaderRow::Column(Column.Key).DefaultLabel(FText::FromString(Column.Value)).FillWidth(Column.Key==TEXT("Facility")||Column.Key==TEXT("Recipe")?2.f:1.f));
        SAssignNew(List,SListView<FJobRow>).ListItemsSource(&Rows).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
            .OnGenerateRow_Lambda([](FJobRow Item,const TSharedRef<STableViewBase>& Owner){return SNew(SIndustryJobRow,Owner).Item(Item);})
            .OnSelectionChanged_Lambda([this](FJobRow Item,ESelectInfo::Type){if(!bRefreshing){Selected=Item?Item->Id:FGuid();Feedback=FText::GetEmpty();}});
        ChildSlot
        [SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text(INVTEXT("YOUR MANUFACTURING JOBS")).Font(MissionUIStyle::GetRowFont(16))]
            +SVerticalBox::Slot().FillHeight(.62f).Padding(8)[MissionUIStyle::MakeDataPane(List.ToSharedRef())]
            +SVerticalBox::Slot().FillHeight(.38f).Padding(8)[MissionUIStyle::MakeDataPane(
                SNew(SScrollBox)+SScrollBox::Slot()[SNew(SChangeDrivenText).Font(MissionUIStyle::GetRowFont(12)).SelectionRevision_Lambda([this](){return GetTypeHash(Selected);}).Text_Lambda([this](){return Details();})])]
            +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([this](){return Feedback;})]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton)
                    .ToolTipText(INVTEXT("Cancel your selected job while docked at its facility. Consumed inputs and licensed runs are not refunded."))
                    .IsEnabled_Lambda([this](){const auto* Job=FindSelected();const auto* Site=FStationServices::Facility();return Catalog.IsValid() && Job && Site && Job->Facility==FName(*Site->Facility) && Job->State!=EIndustryJobState::Completed && Job->State!=EIndustryJobState::Cancelled;})
                    .OnClicked_Lambda([this](){if(Catalog.IsValid())Catalog->RequestCancelPlayerJob(Selected,[Weak=TWeakPtr<SPlayerIndustryJobs>(StaticCastSharedRef<SPlayerIndustryJobs>(AsShared()))](bool,const FText& Why,FGuid){if(auto W=Weak.Pin()){W->Feedback=Why;W->Refresh(0,0);}},Feedback);Refresh(0,0);return FReply::Handled();})
                    [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]]];
        Refresh(0,0);
        ChangeHandle=FSimDataEvents::Changed().AddSP(this,&SPlayerIndustryJobs::OnDataChanged);
    }
    ~SPlayerIndustryJobs(){FSimDataEvents::Changed().Remove(ChangeHandle);}
    void Tick(const FGeometry& G,double Time,float Delta)override{SCompoundWidget::Tick(G,Time,Delta);if(bDataDirty && Time>=NextDispatch){bDataDirty=false;NextDispatch=Time+1;Refresh(0,0);}}
private:
    FDelegateHandle ChangeHandle;bool bDataDirty=false;double NextDispatch=0;
    void OnDataChanged(const FEconomyState& State){if(const auto* Simulation=Sim::GetSim();Simulation && &Simulation->ReadEconomy()==&State)bDataDirty=true;}
    const FIndustryJob* FindSelected() const
    { const auto* Simulation=Sim::GetSim();return Simulation?Simulation->ReadEconomy().Jobs.FindByPredicate([this](const auto& Job){return Job.Id==Selected && Job.Owner==FEconomyLedger::Pilot();}):nullptr; }
    EActiveTimerReturnType Refresh(double,float)
    {
        bRefreshing=true;Rows.Reset();
        if(const auto* Simulation=Sim::GetSim())
            for(const auto& Job:Simulation->ReadEconomy().Jobs)
                if(Job.Owner==FEconomyLedger::Pilot())Rows.Add(MakeShared<FIndustryJob>(Job));
        Rows.StableSort([](const auto& A,const auto& B){const bool AFinished=A->State==EIndustryJobState::Completed||A->State==EIndustryJobState::Cancelled;const bool BFinished=B->State==EIndustryJobState::Completed||B->State==EIndustryJobState::Cancelled;return AFinished!=BFinished?!AFinished:A->Id.ToString()<B->Id.ToString();});
        if(List)
        {
            List->RequestListRefresh();List->ClearSelection();
            for(const auto& Row:Rows)if(Row->Id==Selected){List->SetSelection(Row,ESelectInfo::Direct);break;}
        }
        bRefreshing=false;return EActiveTimerReturnType::Continue;
    }
    FText Details() const
    {
        const auto* Job=FindSelected();const auto* Simulation=Sim::GetSim();
        if(!Job || !Simulation)return Rows.IsEmpty()?INVTEXT("NO PLAYER JOBS. PURCHASE A BLUEPRINT, DELIVER MATERIALS TO PLAYER INPUT AND INSTALL A JOB WHILE DOCKED."):INVTEXT("SELECT A JOB TO VIEW ITS INPUTS, OUTPUTS AND STATUS.");
        TArray<FText> Lines;
        Lines.Add(FText::Format(INVTEXT("JOB {0}\n{1} | {2} | {3} RUNS | {4}\nINPUT: {5}\nOUTPUT: {6}\nDURATION: {7} S | REMAINING: {8} S"),
            FText::FromString(Job->Id.ToString()),FText::FromName(Job->Facility),FText::FromName(Job->Recipe),FText::AsNumber(Job->Runs),JobState(*Job),FText::FromName(Job->Input),FText::FromName(Job->Output),FText::AsNumber(Job->Duration),FText::AsNumber(FMath::CeilToInt(Job->Remaining))));
        if(Job->State!=EIndustryJobState::Completed && Job->State!=EIndustryJobState::Cancelled)
        {FText Reason;FEconomyLedger::CanStartJob(Simulation->ReadEconomy(),*Job,Operational(*Job),Reason);Lines.Add(Reason);}
        double Reserved=0;for(const auto& Pair:Simulation->ReadEconomy().Reservations)
            if(Pair.Value.Inventory==Job->Input && Pair.Value.Owner==Job->Owner)Reserved+=Pair.Value.Quantity;
        Lines.Add(FText::Format(INVTEXT("RESERVED AT INPUT: {0} CATALOG UNITS\nOUTPUT DESTINATION: {1}"),FText::AsNumber(Reserved),FText::FromName(Job->Output)));
        if(Job->State==EIndustryJobState::Cancelled)Lines.Add(INVTEXT("CANCELLED: UNCONSUMED RESERVATIONS RELEASED. CONSUMED MATERIALS, FEES AND LICENSED RUNS ARE NOT REFUNDED."));
        for(const auto& Amount:Job->Materials)Lines.Add(FText::Format(INVTEXT("INPUT {0}: {1} ({2} T)"),FText::FromName(Amount.Item),FText::AsNumber(Amount.Quantity),FText::AsNumber(Amount.Quantity*Amount.UnitTonnes)));
        for(const auto& Amount:Job->Products)Lines.Add(FText::Format(INVTEXT("OUTPUT {0}: {1} ({2} T)"),FText::FromName(Amount.Item),FText::AsNumber(Amount.Quantity),FText::AsNumber(Amount.Quantity*Amount.UnitTonnes)));
        Lines.Add(INVTEXT("OUTPUT IS DEPOSITED AUTOMATICALLY. RUNNING-JOB INPUTS AND LICENSED RUNS ARE NOT REFUNDED ON CANCELLATION."));
        return FText::Join(INVTEXT("\n"),Lines).ToUpper();
    }
    TWeakObjectPtr<UBlueprintCatalogSubsystem> Catalog;
    TArray<FJobRow> Rows;
    TSharedPtr<SListView<FJobRow>> List;
    FGuid Selected;
    FText Feedback;
    bool bRefreshing=false;
};
}
TSharedRef<SWidget> MakePlayerIndustryJobsPanel(UGameInstance* Instance)
{ return SNew(SPlayerIndustryJobs).Instance(Instance); }
