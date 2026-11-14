
#include "ContactsDlg.h"
#include "FlightPanelStyle.h"
#include "MissionUIStyle.h"
#include "ShipVisualCatalog.h"
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
#include "Components/TextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"
#include "ShipDesignRegistry.h"
#include "Styling/CoreStyle.h"

namespace {
const FButtonStyle& ContactButtonStyle()
{
    static const FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateRoundedBoxBrush(FLinearColor(.035f, .075f, .115f), 6.f))
        .SetHovered(FSlateRoundedBoxBrush(FLinearColor(.075f, .21f, .32f), 6.f))
        .SetPressed(FSlateRoundedBoxBrush(FLinearColor(.11f, .28f, .4f), 6.f));
    return Style;
}

struct FContactRowData {
    TWeakObjectPtr<AActor> Actor;
    FString Name, Type;
    FString Description;
    bool bBase = false;
    double Range = 0;
    bool Enemy = false;
    FLinearColor Color = FLinearColor::Yellow;
    TStrongObjectPtr<UTexture2D> Icon;
    bool IconLoadAttempted = false;
    TSharedPtr<FSlateBrush> Brush;
};
using FContactRow = TSharedPtr<FContactRowData>;
class SContactsList : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SContactsList) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UMissionBriefingDlg>, Host)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args) {
        Host = Args._Host;
        ChildSlot[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("NoBrush")).Padding(10)
            [SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
                [SNew(SButton).ButtonStyle(&ContactButtonStyle()).IsFocusable(false).OnClicked_Lambda([this](){EnemiesOnly=!EnemiesOnly;Refresh();return FReply::Handled();})
                    [SNew(STextBlock).Font(MissionUIStyle::GetInfoLabelFont(15)).ColorAndOpacity(FLinearColor::White).Text_Lambda([this](){return FText::FromString(EnemiesOnly?TEXT("ENEMIES - SHOW ALL"):TEXT("ALL CONTACTS - SHOW ENEMIES"));})]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).ButtonStyle(&ContactButtonStyle()).IsFocusable(false).OnClicked_Lambda([this](){Sort(true);return FReply::Handled();})
                        [SNew(STextBlock).Font(MissionUIStyle::GetInfoLabelFont(15)).ColorAndOpacity(FLinearColor::White).Text_Lambda([this](){return FText::FromString(ByName?(Ascending?TEXT("NAME A-Z"):TEXT("NAME Z-A")):TEXT("NAME"));})]]
                    +SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).ButtonStyle(&ContactButtonStyle()).IsFocusable(false).OnClicked_Lambda([this](){Sort(false);return FReply::Handled();})
                        [SNew(STextBlock).Font(MissionUIStyle::GetInfoLabelFont(15)).ColorAndOpacity(FLinearColor::White).Text_Lambda([this](){return FText::FromString(!ByName?(Ascending?TEXT("DISTANCE NEAREST"):TEXT("DISTANCE FARTHEST")):TEXT("DISTANCE"));})]]]
                +SVerticalBox::Slot().FillHeight(1)
                [SAssignNew(List,SListView<FContactRow>).ListItemsSource(&Rows).SelectionMode(ESelectionMode::None)
                    .OnGenerateRow_Lambda([this](FContactRow Row,const TSharedRef<STableViewBase>& Owner)->TSharedRef<ITableRow> {
                        return SNew(STableRow<FContactRow>,Owner).Padding(2)
                            [SNew(SButton).ButtonStyle(&ContactButtonStyle()).IsFocusable(false)
                                .ButtonColorAndOpacity_Lambda([this,Row](){return Host.IsValid() && Host->GetSelectedMissionContact()==Row->Actor.Get()
                                    ?FLinearColor(1.4f,1.8f,2.0f,1):FLinearColor::White;})
                                .OnClicked_Lambda([this,Row](){ Select(Row); return FReply::Handled(); })
                                [SNew(SHorizontalBox)
                                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                                    [SNew(SBox).WidthOverride(48).HeightOverride(48)
                                        [SNew(SImage).ColorAndOpacity(FLinearColor::White)
                                        .Image_Lambda([Row](){return Row->Brush.IsValid()?Row->Brush.Get():FCoreStyle::Get().GetBrush("WhiteBrush");})]]
                                    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                                    [SNew(STextBlock).Font(MissionUIStyle::GetInfoValueFont(16)).ColorAndOpacity_Lambda([Row](){return Row->Color;}).AutoWrapText(true)
                                    .Text_Lambda([this,Row](){return FText::FromString(FString::Printf(TEXT("%s%s\n%s  |  %s"),
                                        Host.IsValid() && Host->GetSelectedMissionContact()==Row->Actor.Get()?TEXT("> "):TEXT(""),*Row->Name,*Row->Type,*UFormattingUtils::FormatDistance(Row->Range * 1000.0)));})]]];
                    })]
                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("%d CONTACTS"),Rows.Num()));})]
                +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
                [SNew(SBorder).Padding(10)
                    .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(FLinearColor(.035f,.075f,.115f,1))
                    .Visibility_Lambda([this](){return SelectedDetails() ? EVisibility::Visible : EVisibility::Collapsed;})
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
                        [SNew(STextBlock).Font(MissionUIStyle::GetInfoLabelFont(15)).AutoWrapText(true)
                            .Text_Lambda([this](){const auto Row = SelectedDetails(); return Row ? FText::FromString(Row->Name) : FText::GetEmpty();})]
                        +SVerticalBox::Slot().AutoHeight()
                        [SNew(SBox).MaxDesiredHeight(112)
                            [SNew(SScrollBox)
                                +SScrollBox::Slot()
                                [SNew(STextBlock).AutoWrapText(true)
                                    .Text_Lambda([this](){
                                        const auto Row = SelectedDetails();
                                        if (!Row) return FText::GetEmpty();
                                        FString Details = Row->Type + TEXT("  |  ") + UFormattingUtils::FormatDistance(Row->Range * 1000);
                                        if (!Row->Description.IsEmpty()) Details += TEXT("\n\n") + Row->Description;
                                        return FText::FromString(Details);
                                    })]]]]]]];
        Refresh();
    }
    virtual void Tick(const FGeometry& G,double Time,float Delta) override {
        SCompoundWidget::Tick(G,Time,Delta);
        if(Time>=NextRefresh){NextRefresh=Time+0.5;Refresh();}
    }
    void MoveSelection(int32 Step)
    {
        if (!Host.IsValid() || Rows.IsEmpty()) return;
        const int32 Index = Rows.IndexOfByPredicate([this](const FContactRow& Row)
        {
            return Row->Actor.Get() == Host->GetSelectedMissionContact();
        });
        const int32 Next = Index == INDEX_NONE ? (Step > 0 ? 0 : Rows.Num() - 1)
            : FMath::Clamp(Index + Step, 0, Rows.Num() - 1);
        Select(Rows[Next]);
        List->RequestScrollIntoView(Rows[Next]);
    }
private:
    TWeakObjectPtr<UMissionBriefingDlg> Host;
    TArray<FContactRow> Rows;
    TSharedPtr<SListView<FContactRow>> List;

    void Select(const FContactRow& Row)
    {
        if (Host.IsValid() && Row->Actor.IsValid())
            Host->SelectMissionContact(Row->Actor.Get());
    }

    FContactRow SelectedDetails() const
    {
        if (!Host.IsValid()) return {};
        const auto* Actor = Host->GetSelectedMissionContact();
        for (const auto& Row : Rows)
            if (Row->Actor.IsValid() && Row->Actor.Get() == Actor &&
                (Row->bBase || !Row->Description.IsEmpty())) return Row;
        return {};
    }

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
            Row->bBase = false;
            Row->Description.Reset();
            Row->Enemy=false;Row->Color=FLinearColor::Yellow;
            AShipActor* Visual=Cast<AShipActor>(Actor);
            Ship* Data=Visual?Visual->GetRuntimeShip():nullptr;
            if(Data && Region->GetShips().contains(Data)) {
                Row->Name=ANSI_TO_TCHAR(Data->GetName());
                Row->Type=ANSI_TO_TCHAR(Data->GetShipClassName());
                Row->bBase = Data->GetClassification() == CLASSIFICATION::STATION ||
                    Data->GetClassification() == CLASSIFICATION::STARBASE;
                if (Data->Design())
                {
                    if (const auto* Design = ShipDesignRegistry::Find(Data->Design()->name))
                        Row->Description = Design->Description;
                    const auto& Capacity = Data->Design()->CargoCapacity;
                    if (Capacity.Tonnes > 0)
                        Row->Description += FString::Printf(TEXT("\n\nCargo capacity  %.0f t"), Capacity.Tonnes);
                }
                Row->Range=FVector::Distance(Data->GetLocation(),Player->GetLocation())/1000.0;
                const int Team=Player->GetIFF();
                Row->Enemy=Data->GetIFF()>0 && Data->GetIFF()!=Team;
                if(Data->GetIFF()>0)Row->Color=Row->Enemy?FLinearColor::Red:FLinearColor(0.25f,0.65f,1,1);
                if(!Row->IconLoadAttempted){
                    Row->IconLoadAttempted = true;
                    FString Path;
                    if(const FShipDesign* D = Data->Design() ? ShipDesignRegistry::Find(Data->Design()->name) : nullptr){
                        if(!D->HudIconName.IsEmpty()){
                            FString Icon = D->HudIconName;
                            if(Icon.StartsWith(TEXT("/Game/"))) Path = Icon;
                            else { Icon.RemoveFromEnd(TEXT(".pcx"), ESearchCase::IgnoreCase);
                                   Path = FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *ShipVisualCatalog::IconFolder(D->ShipName), *Icon, *Icon); }
                        }
                    }
                    if(Path.IsEmpty()){
                        const FString ShipName = Row->Type;
                        Path = FString::Printf(TEXT("/Game/UI/Ships/%s/%s.%s"), *ShipName, *ShipName, *ShipName);
                    }
                    const TCHAR* StationIcon = Data->Design()
                        ? ShipVisualCatalog::DedicatedStationIcon(ANSI_TO_TCHAR(Data->Design()->name)) : nullptr;
                    if (StationIcon)
                        Path = StationIcon;
                    else if (Data->GetClassification() == CLASSIFICATION::STARBASE)
                        Path = AirbaseContactIcon;
                    else if (Data->GetClassification() == CLASSIFICATION::STATION)
                        Path = StationContactIcon;
                    else if (Data->GetClassification() == CLASSIFICATION::FARCASTER)
                        Path = TEXT("/Game/UI/Ships/Farcaster/FarcasterHud.FarcasterHud");
                    Row->Icon.Reset(LoadObject<UTexture2D>(nullptr, *Path));
                    if(Row->Icon.IsValid()){
                        Row->Brush = MakeShared<FSlateBrush>();
                        Row->Brush->SetResourceObject(Row->Icon.Get());
                        Row->Brush->ImageSize = FVector2D(48,48);
                        Row->Brush->DrawAs = ESlateBrushDrawType::Image;
                    }
                }
            } else {
                FMissionObjectData Object;
                if(!ReadMissionObject(Actor,Player,Region,Object))continue;
                Row->Name=Object.Name;Row->Type=Object.Type;Row->Range=Object.RangeKm;
                Row->bBase = Object.ImagePath == AirbaseContactIcon || Object.ImagePath == StationContactIcon;
                if (!Row->IconLoadAttempted) {
                    Row->IconLoadAttempted = true;
                    FString ImagePath = Object.ImagePath;
                    if (ImagePath.IsEmpty() && Cast<AWaypointActor>(Actor))
                        ImagePath = TEXT("/Game/UI/HUD/lead.lead");
                    if (!ImagePath.IsEmpty())
                        Row->Icon.Reset(LoadObject<UTexture2D>(nullptr, *ImagePath));
                    if (Row->Icon.IsValid()) {
                        Row->Brush = MakeShared<FSlateBrush>();
                        Row->Brush->SetResourceObject(Row->Icon.Get());
                        Row->Brush->ImageSize = FVector2D(48,48);
                        Row->Brush->DrawAs = ESlateBrushDrawType::Image;
                    }
                }
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
void UContactsDlg::NativeConstruct()
{
    ContentInsets.Top = 4;
    Super::NativeConstruct();
    if (PanelTitle) PanelTitle->SetVisibility(ESlateVisibility::Collapsed);
}
FReply UContactsDlg::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const auto List = StaticCastSharedPtr<SContactsList>(ContactList.Pin());
    const FKey Key = Event.GetKey();
    if (Key == EKeys::Escape) return Super::NativeOnPreviewKeyDown(Geometry, Event);
    if (Key == EKeys::Up || Key == EKeys::Down)
    {
        if (List) List->MoveSelection(Key == EKeys::Up ? -1 : 1);
        return FReply::Handled();
    }
    if (!Event.IsRepeat())
    {
        if (UMissionBriefingDlg* Host = MissionHost.Get(); Host && Host->HandleContactsRadioKey(Key))
            return FReply::Handled();
        RequestPanelClose();
    }
    return FReply::Handled();
}
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
TSharedRef<SWidget> UContactsDlg::CreatePanelContent()
{
    TSharedRef<SContactsList> List = SNew(SContactsList).Host(MissionHost);
    ContactList = List;
    return List;
}
FText UContactsDlg::GetPanelCaption() const{return FText::FromString(TEXT("CONTACTS"));}
