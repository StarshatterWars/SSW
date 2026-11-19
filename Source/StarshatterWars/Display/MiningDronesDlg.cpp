#include "MiningDronesDlg.h"
#include "MiningDroneSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Text/STextBlock.h"
namespace
{
struct FDroneRow{FMiningDroneState Drone;int32 Number=0;};
using FDroneRowPtr=TSharedPtr<FDroneRow>;
class SDroneRow final:public SMultiColumnTableRow<FDroneRowPtr>
{
public:
    SLATE_BEGIN_ARGS(SDroneRow){}SLATE_ARGUMENT(FDroneRowPtr,Entry)SLATE_END_ARGS()
    void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
    {Entry=Args._Entry;SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner);}
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        FText Text;
        if(Column==TEXT("Drone"))Text=FText::Format(INVTEXT("DRONE {0}"),FText::AsNumber(Entry->Number));
        else if(Column==TEXT("State"))Text=FText::FromString(StaticEnum<EMiningDroneState>()->GetNameStringByValue(static_cast<int64>(Entry->Drone.State)).ToUpper()+(Entry->Drone.bRepeatMining?TEXT(" / REPEAT"):TEXT("")));
        else if(Column==TEXT("Ore"))Text=FText::FromString(Entry->Drone.Ore.IsNone()?TEXT("EMPTY"):FString::Printf(TEXT("%s  %.4f T"),*Entry->Drone.Ore.ToString().ToUpper(),Entry->Drone.OreTonnes));
        else Text=Entry->Drone.TargetContact>=0?FText::AsNumber(Entry->Drone.TargetContact):INVTEXT("-");
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Text);
    }
private:FDroneRowPtr Entry;
};
class SMiningDronePanel final:public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMiningDronePanel){}SLATE_ARGUMENT(UGameInstance*,Instance)SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Service=Args._Instance?Args._Instance->GetSubsystem<UMiningDroneSubsystem>():nullptr;
        if(Service.IsValid())Service->PrepareBay(Feedback);
        auto Header=SNew(SHeaderRow)+SHeaderRow::Column(TEXT("Drone")).DefaultLabel(INVTEXT("DRONE")).FillWidth(1)
            +SHeaderRow::Column(TEXT("State")).DefaultLabel(INVTEXT("STATE")).FillWidth(1.5)
            +SHeaderRow::Column(TEXT("Ore")).DefaultLabel(INVTEXT("ONBOARD ORE")).FillWidth(2)
            +SHeaderRow::Column(TEXT("Target")).DefaultLabel(INVTEXT("ASTEROID ID")).FillWidth(1);
        SAssignNew(DroneList,SListView<FDroneRowPtr>).ListItemsSource(&Rows).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
            .OnGenerateRow_Lambda([](FDroneRowPtr Row,const TSharedRef<STableViewBase>& Owner){return SNew(SDroneRow,Owner).Entry(Row);})
            .OnSelectionChanged_Lambda([this](FDroneRowPtr Row,ESelectInfo::Type){if(!bRefreshing)Selected=Row?Row->Drone.Id:FGuid();});
        auto Actions=SNew(SHorizontalBox);
        const EShipSystemIcon Icons[]={EShipSystemIcon::Play,EShipSystemIcon::Mineral,EShipSystemIcon::Abort,EShipSystemIcon::Navigation,EShipSystemIcon::Station,EShipSystemIcon::Refresh};
        const FText Tips[]={INVTEXT("Launch selected drone; control bandwidth applies"),INVTEXT("Mine the selected assayed asteroid within control range"),INVTEXT("Stop mining and hold position"),INVTEXT("Return to owner and unload ore"),INVTEXT("Dock within 30 m; ore stays aboard if your hold is full"),INVTEXT("Repeat mining the selected deposit: return, unload, then resume until depleted or your hold is full")};
        for(int32 Action=0;Action<6;++Action)
            Actions->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).ToolTipText(Tips[Action])
                .IsEnabled_Lambda([this](){return Service.IsValid() && Service->HasBay();}).OnClicked_Lambda([this,Action](){Run(Action);return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(Icons[Action])]];
        Actions->AddSlot().AutoWidth().Padding(0,0,8,0)
            [SNew(SButton).ToolTipText_Lambda([this]()
                { FText Reason; if (Service.IsValid()) Service->CanLoadSpare(Reason); return Reason; })
                .IsEnabled_Lambda([this]() { FText Reason; return Service.IsValid() && Service->CanLoadSpare(Reason); })
                .OnClicked_Lambda([this]() { if (Service.IsValid()) Service->LoadSpare(Feedback); Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Replace)]];
        ChildSlot[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text(INVTEXT("MINING DRONE CONTROL | ORE ENTERS YOUR HOLD ONLY AFTER RETURN AND UNLOAD. LOST DRONE CARGO IS LOST."))]
            +SVerticalBox::Slot().FillHeight(1)[DroneList.ToSharedRef()]
            +SVerticalBox::Slot().AutoHeight().Padding(0,12)[Actions]
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([this](){return Feedback.IsEmpty() && Service.IsValid()?Service->GetStatusText():Feedback;})])];
        Refresh();RegisterActiveTimer(.5f,FWidgetActiveTimerDelegate::CreateLambda([this](double,float){Refresh();return EActiveTimerReturnType::Continue;}));
    }
private:
    void Run(int32 Action)
    {
        if(!Service.IsValid())return;
        switch(Action){case 0:Service->Launch(Selected,Feedback);break;case 1:Service->Mine(Selected,Feedback);break;case 2:Service->Stop(Selected,Feedback);break;case 3:Service->Return(Selected,Feedback);break;case 4:Service->Dock(Selected,Feedback);break;case 5:Service->MineRepeatedly(Selected,Feedback);break;}Refresh();
    }
    void Refresh()
    {
        bRefreshing=true;Rows.Reset();int32 Number=0;if(Service.IsValid())for(const auto& Drone:Service->GetDrones()){auto Row=MakeShared<FDroneRow>();Row->Drone=Drone;Row->Number=++Number;Rows.Add(Row);}
        DroneList->RequestListRefresh();for(const auto& Row:Rows)if(Row->Drone.Id==Selected){DroneList->SetSelection(Row);break;}bRefreshing=false;
    }
    TWeakObjectPtr<UMiningDroneSubsystem> Service;TSharedPtr<SListView<FDroneRowPtr>> DroneList;TArray<FDroneRowPtr> Rows;FGuid Selected;FText Feedback;bool bRefreshing=false;
};
}
TSharedRef<SWidget> UMiningDronesDlg::RebuildWidget(){return SNew(SMiningDronePanel).Instance(GetGameInstance());}
