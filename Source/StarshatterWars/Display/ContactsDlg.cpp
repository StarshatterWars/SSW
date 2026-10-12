#include "ContactsDlg.h"
#include "FormattingUtils.h"
#include "MissionBriefingDlg.h"
#include "MissionObjectTarget.h"
#include "ShipActor.h"
#include "Sim.h"
#include "Mission.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Styling/CoreStyle.h"

namespace {
struct FContactRowData {
    TWeakObjectPtr<AActor> Actor;
    FString Name, Type;
    double Range = 0;
    bool Enemy = false;
    FLinearColor Color = FLinearColor::Yellow;
};
using FContactRow = TSharedPtr<FContactRowData>;
class SContactsList : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SContactsList) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UMissionBriefingDlg>, Host)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {
        Host = Args._Host;
        ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(FLinearColor(0.01f,0.02f,0.035f,0.96f)).Padding(10)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [SNew(SButton).IsFocusable(false).OnClicked_Lambda([this](){EnemiesOnly=!EnemiesOnly;Refresh();return FReply::Handled();})
                    [SNew(STextBlock).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return FText::FromString(EnemiesOnly?TEXT("ENEMIES - SHOW ALL"):TEXT("ALL CONTACTS - SHOW ENEMIES"));})]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).IsFocusable(false).OnClicked_Lambda([this](){Sort(true);return FReply::Handled();})
                        [SNew(STextBlock).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return FText::FromString(ByName?(Ascending?TEXT("NAME A-Z"):TEXT("NAME Z-A")):TEXT("NAME"));})]]
                    +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).IsFocusable(false).OnClicked_Lambda([this](){Sort(false);return FReply::Handled();})
                        [SNew(STextBlock).ColorAndOpacity(FLinearColor::Black).Text_Lambda([this](){return FText::FromString(!ByName?(Ascending?TEXT("DISTANCE NEAREST"):TEXT("DISTANCE FARTHEST")):TEXT("DISTANCE"));})]]]
                +SVerticalBox::Slot().FillHeight(1)
                [SAssignNew(List,SListView<FContactRow>).ListItemsSource(&Rows).SelectionMode(ESelectionMode::None)
                    .OnGenerateRow_Lambda([this](FContactRow Row,const TSharedRef<STableViewBase>& Owner)->TSharedRef<ITableRow> {
                        return SNew(STableRow<FContactRow>,Owner).Padding(2)
                            [SNew(SButton).IsFocusable(false)
                                .ButtonColorAndOpacity_Lambda([this,Row](){return Host.IsValid() && Host->GetSelectedMissionContact()==Row->Actor.Get()
                                    ?FLinearColor(0.2f,0.45f,0.65f,1):FLinearColor(0.035f,0.035f,0.035f,1);})
                                .OnClicked_Lambda([this,Row](){if(Host.IsValid() && Row->Actor.IsValid())Host->SelectMissionContact(Row->Actor.Get());return FReply::Handled();})
                                [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity_Lambda([Row](){return Row->Color;}).AutoWrapText(true)
                                    .Text_Lambda([this,Row](){return FText::FromString(FString::Printf(TEXT("%s%s\n%s  |  %s"),
                                        Host.IsValid() && Host->GetSelectedMissionContact()==Row->Actor.Get()?TEXT("> "):TEXT(""),*Row->Name,*Row->Type,*UFormattingUtils::FormatDistance(Row->Range * 1000.0)));})]];
                    })]
                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("%d CONTACTS"),Rows.Num()));})]]];
        Refresh();
    }
    virtual void Tick(const FGeometry& G,double Time,float Delta) override {
        SCompoundWidget::Tick(G,Time,Delta);
        if(Time>=NextRefresh){NextRefresh=Time+0.5;Refresh();}
    }
private:
    TWeakObjectPtr<UMissionBriefingDlg> Host;
    TArray<FContactRow> Rows;
    TSharedPtr<SListView<FContactRow>> List;
    bool EnemiesOnly=false,ByName=false,Ascending=true;
    double NextRefresh=0;
    void Sort(bool Name){if(ByName==Name)Ascending=!Ascending;else{ByName=Name;Ascending=true;}Refresh();}
    void Refresh() {
        TArray<FContactRow> Updated;
        Sim* Simulation=Sim::GetSim();
        Ship* Player=Simulation?Simulation->GetPlayerShip():nullptr;
        SimRegion* Region=Simulation?Simulation->GetActiveRegion():nullptr;
        if(Host.IsValid() && Player && Region) for(AActor* Actor:Host->GetSelectableMissionContacts()) {
            FContactRow Row;
            for(const FContactRow& Old:Rows)if(Old->Actor.Get()==Actor){Row=Old;break;}
            if(!Row)Row=MakeShared<FContactRowData>();
            Row->Actor=Actor;
            Row->Enemy=false;Row->Color=FLinearColor::Yellow;
            AShipActor* Visual=Cast<AShipActor>(Actor);
            Ship* Data=Visual?Visual->GetRuntimeShip():nullptr;
            if(Data && Region->GetShips().contains(Data)) {
                Row->Name=ANSI_TO_TCHAR(Data->GetName());
                Row->Type=ANSI_TO_TCHAR(Data->GetShipClassName());
                Row->Range=FVector::Distance(Data->GetLocation(),Player->GetLocation())/1000.0;
                const int Team=Simulation->GetMission()?Simulation->GetMission()->GetTeam():Player->GetIFF();
                Row->Enemy=Data->GetIFF()>0 && Data->GetIFF()!=Team;
                if(Data->GetIFF()>0)Row->Color=Row->Enemy?FLinearColor::Red:FLinearColor(0.25f,0.65f,1,1);
            } else {
                FMissionObjectData Object;
                if(!ReadMissionObject(Actor,Player,Region,Object))continue;
                Row->Name=Object.Name;Row->Type=Object.Type;Row->Range=Object.RangeKm;
            }
            if(!EnemiesOnly || Row->Enemy)Updated.Add(Row);
        }
        Updated.StableSort([this](const FContactRow& A,const FContactRow& B){
            int Order=ByName?A->Name.Compare(B->Name,ESearchCase::IgnoreCase):(A->Range<B->Range?-1:(A->Range>B->Range?1:0));
            if(Order)return Ascending?Order<0:Order>0;
            return A->Actor->GetUniqueID()<B->Actor->GetUniqueID();
        });
        const bool Changed=Rows!=Updated;
        Rows=MoveTemp(Updated);
        if(Changed && List.IsValid())List->RequestListRefresh();
    }
};
}

void UContactsDlg::SetMissionHost(UMissionBriefingDlg* Host){MissionHost=Host;}
TSharedRef<SWidget> UContactsDlg::RebuildWidget() {
    // Preserve the authored frame; supply a missing content host in a new Blueprint.
    if(WidgetTree && WidgetTree->RootWidget && !WidgetTree->FindWidget(TEXT("RuntimeHost"))) {
        UCanvasPanel* Canvas=Cast<UCanvasPanel>(WidgetTree->RootWidget);
        if(!Canvas){
            UWidget* Original=WidgetTree->RootWidget;
            Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();
            WidgetTree->RootWidget=Canvas;
            auto* FrameSlot=Canvas->AddChildToCanvas(Original);
            FrameSlot->SetAnchors(FAnchors(0,0,1,1));FrameSlot->SetOffsets(FMargin(0));
        }
        RuntimeHost=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("RuntimeHost"));
        Canvas->AddChildToCanvas(RuntimeHost)->SetZOrder(5);
    }
    return Super::RebuildWidget();
}
TSharedRef<SWidget> UContactsDlg::CreatePanelContent(){return SNew(SContactsList).Host(MissionHost);}
FText UContactsDlg::GetPanelCaption() const{return FText::FromString(TEXT("Contacts"));}
