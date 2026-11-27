#include "SkillsDlg.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Rendering/DrawElements.h"
#include "RecordDetails.h"
#include "Widgets/Layout/SScrollBox.h"
#include "PlayerCharacter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "SkillsSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Views/STableRow.h"

namespace
{
class SSkillLevelMark : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SSkillLevelMark){} SLATE_ATTRIBUTE(bool,Trained) SLATE_END_ARGS()
    void Construct(const FArguments& Args){Trained=Args._Trained;}
    FVector2D ComputeDesiredSize(float) const override{return FVector2D(14,18);}
    int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override
    {
        const bool Complete=Trained.Get(false);TArray<FVector2D> Points;const FVector2D Center=G.GetLocalSize()*.5;
        const int32 Count=Complete?10:16;
        for(int32 I=0;I<=Count;++I){const double Angle=-PI*.5+I*2*PI/Count;const double Radius=Complete?(I%2?2.0:5.0):1.6;Points.Add(Center+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius);}
        const FLinearColor Tint=(Complete?FLinearColor(.45f,.85f,1.f):FLinearColor(.45f,.55f,.62f))*Style.GetColorAndOpacityTint();
        FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Tint,true,Complete?1.2f:2.f);return Layer;
    }
private:TAttribute<bool> Trained;
};
class SSkillQueue : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSkillQueue){}SLATE_ARGUMENT(TWeakObjectPtr<USkillsSubsystem>,Service)SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Service=Args._Service;
        ChildSlot[SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("TRAINING QUEUE / 30-DAY HORIZON")).Font(MissionUIStyle::GetHeaderFont(12))]
            +SVerticalBox::Slot().AutoHeight().Padding(0,6)[SNew(SBox).MaxDesiredHeight(118)[SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(Rows,SVerticalBox)]]]
            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(10)).AutoWrapText(true).Text_Lambda([this](){return Feedback;})]];
        Refresh();RegisterActiveTimer(.5f,FWidgetActiveTimerDelegate::CreateLambda([this](double,float){Refresh();return EActiveTimerReturnType::Continue;}));
    }
private:
    TFunction<void(bool,const FText&)> Completion()
    {
        const TWeakPtr<SSkillQueue> Weak=StaticCastSharedRef<SSkillQueue>(AsShared());
        return [Weak](bool,const FText& Why){if(auto Widget=Weak.Pin()){Widget->Feedback=Why;Widget->Refresh();}};
    }
    void Refresh()
    {
        const auto Queue=Service.IsValid()?Service->GetQueue():TConstArrayView<FSkillTrainingEntry>();FString Signature;
        for(const auto& E:Queue)Signature+=E.Id.ToString();if(Signature==Previous && bBuilt)return;Previous=Signature;bBuilt=true;Rows->ClearChildren();
        if(Queue.IsEmpty())Rows->AddSlot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("Click an untrained level dot to add training.")).Font(MissionUIStyle::GetRowFont(11))];
        for(const auto& E:Queue)
        {
            const auto* Def=Service->GetCatalog().Find(E.Skill);const FText Title=FText::FromString(Def?Def->DisplayName:E.Skill.ToString());
            Rows->AddSlot().AutoHeight().Padding(0,3)[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)
                    .Text_Lambda([E,Title](){const int64 Left=E.FinishesUtc-FDateTime::UtcNow().ToUnixTimestamp();return FText::Format(INVTEXT("{0} / LEVEL {1}  •  {2}\nSTART {3} GMT  /  FINISH {4} GMT"),Title,FText::AsNumber(E.TargetLevel),Left>0?FText::FromString(FTimespan::FromSeconds(Left).ToString()):INVTEXT("VERIFYING PREREQUISITES"),FText::FromString(FDateTime::FromUnixTimestamp(E.StartedUtc).ToString(TEXT("%d %b %H:%M"))),FText::FromString(FDateTime::FromUnixTimestamp(E.FinishesUtc).ToString(TEXT("%d %b %H:%M"))));})]
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text_Lambda([E](){return FText::Format(INVTEXT("{0} CR"),FText::AsNumber(FSkillTrainingRules::InstantCost(E,FDateTime::UtcNow().ToUnixTimestamp())));})]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Instant-train this level for credits. Click once to request, then confirm."))
                    .OnClicked_Lambda([this,Id=E.Id](){if(PendingInstant==Id){if(Service.IsValid())Service->InstantTrain(Id,Feedback,Completion());PendingInstant.Invalidate();}else{PendingInstant=Id;Feedback=INVTEXT("Click the check mark beside this entry to confirm instant training for the displayed credits.");}Refresh();return FReply::Handled();})
                    [SNew(SWidgetSwitcher).WidgetIndex_Lambda([this,Id=E.Id](){return PendingInstant==Id?1:0;})
                        +SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Play)]
                        +SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]]]
                +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).ToolTipText(INVTEXT("Cancel this queued level. Active progress is retained."))
                    .OnClicked_Lambda([this,Id=E.Id](){if(Service.IsValid())Service->CancelTraining(Id,Feedback,Completion());Refresh();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]];
        }
    }
    TWeakObjectPtr<USkillsSubsystem> Service;TSharedPtr<SVerticalBox> Rows;FGuid PendingInstant;FString Previous;FText Feedback;bool bBuilt=false;
};
EShipSystemIcon CategoryIcon(FName Group)
{
    if (Group==TEXT("Gunnery") || Group==TEXT("Missile Systems")) return EShipSystemIcon::Weapon;
    if (Group==TEXT("Navigation")) return EShipSystemIcon::Navigation;
    if (Group==TEXT("Engineering")) return EShipSystemIcon::Reactor;
    if (Group==TEXT("Industry")) return EShipSystemIcon::Production;
    if (Group==TEXT("Mining") || Group==TEXT("Sensors")) return EShipSystemIcon::Sensor;
    if (Group==TEXT("Logistics")) return EShipSystemIcon::Transport;
    if (Group==TEXT("Trade")) return EShipSystemIcon::Marketplace;
    if (Group==TEXT("Spacecraft Command")) return EShipSystemIcon::Ship;
    return EShipSystemIcon::Agent;
}
DECLARE_DELEGATE_TwoParams(FRequestSkillTraining,FName,int32);
using FSkillItem = TSharedPtr<FSkillDefinition>;
class SSkillRow : public SMultiColumnTableRow<FSkillItem>
{
public:
    SLATE_BEGIN_ARGS(SSkillRow) {} SLATE_ARGUMENT(FSkillItem, Item) SLATE_ARGUMENT(TWeakObjectPtr<USkillsSubsystem>, Service) SLATE_EVENT(FRequestSkillTraining,OnRequestTraining) SLATE_END_ARGS()
    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    { Item = Args._Item; Service = Args._Service;RequestTraining=Args._OnRequestTraining; SMultiColumnTableRow<FSkillItem>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner); }
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        if (Column==TEXT("Group")) return SNew(SBox).Padding(6).ToolTipText(FText::FromName(Item->Group))[SNew(SShipSystemIcon).Icon(CategoryIcon(Item->Group))];
        if(Column==TEXT("Levels"))
        {
            auto Marks=SNew(SHorizontalBox);
            for(int32 I=1;I<=10;++I)Marks->AddSlot().AutoWidth().Padding(1,0)[SNew(SButton).ContentPadding(FMargin(1))
                .Visibility_Lambda([Weak=Service,Skill=Item->Name,I](){return I<6 || (Weak.IsValid() && Weak->TrainingCap(Skill)==10)?EVisibility::Visible:EVisibility::Collapsed;})
                .ToolTipText_Lambda([Weak=Service,Skill=Item->Name,I](){return FText::Format(INVTEXT("LEVEL {0} / {1}\n{2}"),FText::AsNumber(I),FText::FromString(FTimespan::FromSeconds(FSkillTrainingRules::Duration(I)).ToString()),Weak.IsValid() && Weak->GetLevel(Skill)>=I?INVTEXT("TRAINED"):INVTEXT("CLICK TO REQUEST TRAINING THROUGH THIS LEVEL"));})
                .OnClicked_Lambda([Request=RequestTraining,Skill=Item->Name,I](){Request.ExecuteIfBound(Skill,I);return FReply::Handled();})
                [SNew(SSkillLevelMark).Trained_Lambda([Weak=Service,Skill=Item->Name,I](){return Weak.IsValid() && Weak->GetLevel(Skill)>=I;})]];
            return Marks;
        }
        if (Column==TEXT("Status")) return SNew(SBox).Padding(6)
            .ToolTipText_Lambda([Item=Item,Weak=Service]() { if (Weak.IsValid() && Weak->HasSkill(Item->Name)) return INVTEXT("Skill complete"); FText Reason; if (Weak.IsValid()) Weak->CanLearn(Item->Name,Reason); return Reason; })
            [SNew(SWidgetSwitcher).WidgetIndex_Lambda([Item=Item,Weak=Service]() { if (Weak.IsValid() && Weak->HasSkill(Item->Name)) return 0; FText Reason; return Weak.IsValid() && Weak->CanLearn(Item->Name,Reason) ? 1 : 2; })
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]];
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Margin(FMargin(6,4)).OverflowPolicy(ETextOverflowPolicy::Ellipsis).ToolTipText(FText::FromString(Item->DisplayName))
            .Text_Lambda([Item = Item, Weak = Service, Column]()
            {
                if (Column == TEXT("Group")) return FText::FromName(Item->Group).ToUpper();
                if (Column == TEXT("Cost")) return FText::Format(INVTEXT("{0} CR"), FText::AsNumber(Item->BookPrice));
                if (Column == TEXT("Status")) return Weak.IsValid() && Weak->HasSkill(Item->Name) ? INVTEXT("COMPLETE") : INVTEXT("NOT LEARNED");
                return FText::FromString(Item->DisplayName).ToUpper();
            });
    }
private: FSkillItem Item; TWeakObjectPtr<USkillsSubsystem> Service;FRequestSkillTraining RequestTraining;
};
struct FSkillsTable
{
    TArray<FSkillItem> All, Visible;
    TWeakPtr<SListView<FSkillItem>> List;
    FSkillItem Selected;
    FName SelectedGroup;
    TWeakObjectPtr<USkillsSubsystem> Service;
    FString Search;
    FName SortColumn = TEXT("Group");
    EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
    FText Feedback;
    int32 PendingLevel=0;
    void Refresh()
    {
        Visible.Reset();
        for (const auto& Item : All)
            if ((SelectedGroup.IsNone() || Item->Group==SelectedGroup) && (Search.IsEmpty() || Search==TEXT("*") || Item->DisplayName.Contains(Search) || Item->Description.Contains(Search) || Item->Group.ToString().Contains(Search))) Visible.Add(Item);
        Visible.StableSort([this](const auto& A, const auto& B)
        {
            int32 Order = 0;
            if (SortColumn == TEXT("Cost")) Order = A->BookPrice < B->BookPrice ? -1 : A->BookPrice > B->BookPrice ? 1 : 0;
            else if (SortColumn == TEXT("Group")) Order = A->Group.ToString().Compare(B->Group.ToString(), ESearchCase::IgnoreCase);
            else if (SortColumn==TEXT("Status"))
            {
                const auto Status=[this](const auto& Item) { if (Service.IsValid() && Service->HasSkill(Item->Name)) return 0; FText Reason; return Service.IsValid() && Service->CanLearn(Item->Name,Reason) ? 1 : 2; };
                Order=Status(A)-Status(B);
            }
            if (!Order) Order = A->DisplayName.Compare(B->DisplayName, ESearchCase::IgnoreCase);
            return SortMode == EColumnSortMode::Descending ? Order > 0 : Order < 0;
        });
        if (auto View=List.Pin()) View->RequestListRefresh();
    }
};
}
TSharedRef<SWidget> USkillsDlg::RebuildWidget()
{
    auto* Service = GetGameInstance()->GetSubsystem<USkillsSubsystem>();
    if (!Service) return SNew(STextBlock).Text(INVTEXT("SKILLS UNAVAILABLE"));
    const TWeakObjectPtr<USkillsSubsystem> Weak(Service);
    auto Table = MakeShared<FSkillsTable>();
    Table->Service=Service;
    for (const auto& Pair : Service->GetCatalog()) Table->All.Add(MakeShared<FSkillDefinition>(Pair.Value));
    Table->Refresh();
    auto Header = SNew(SHeaderRow);
    for (const auto& Column : {TPair<FName,FString>(TEXT("Group"),TEXT("GROUP")), {TEXT("Name"),TEXT("SKILL")}, {TEXT("Levels"),TEXT("LEVELS")}, {TEXT("Status"),TEXT("STATUS")}, {TEXT("Cost"),TEXT("BOOK PRICE")}})
        {
        auto Definition=SHeaderRow::Column(Column.Key).DefaultLabel(FText::FromString(Column.Value));
        if(Column.Key==TEXT("Name"))Definition.FillWidth(1.f);
        else Definition.FixedWidth(Column.Key==TEXT("Group")?56.f:Column.Key==TEXT("Levels")?230.f:Column.Key==TEXT("Status")?66.f:110.f);
        Header->AddColumn(Definition
            .SortMode_Lambda([Table, Id = Column.Key]() { return Table->SortColumn == Id ? Table->SortMode : EColumnSortMode::None; })
            .OnSort_Lambda([Table](EColumnSortPriority::Type, const FName& Id, EColumnSortMode::Type Mode) { Table->SortColumn = Id; Table->SortMode = Mode; Table->Refresh(); }));
    }
    TSet<FName> UniqueGroups;for(const auto& Skill:Table->All)UniqueGroups.Add(Skill->Group);
    TArray<FName> Groups=UniqueGroups.Array();Groups.Sort(FNameLexicalLess());Groups.Insert(NAME_None,0);
    auto GroupButtons=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5,5));
    for(FName Group:Groups)
    {
        const FText Caption=Group.IsNone()?INVTEXT("ALL SKILLS"):FText::FromName(Group).ToUpper();
        GroupButtons->AddSlot()[SNew(SBox).WidthOverride(184).HeightOverride(36)[SNew(SButton).ContentPadding(FMargin(8,5))
            .ToolTip(MissionUIStyle::MakeToolTip(Caption))
            .ButtonColorAndOpacity_Lambda([Table,Group](){return Table->SelectedGroup==Group?MissionUIStyle::RowSelectedBG:MissionUIStyle::RowBG;})
            .OnClicked_Lambda([Table,Group](){Table->SelectedGroup=Group;Table->Selected.Reset();Table->PendingLevel=0;Table->Feedback=FText::GetEmpty();Table->Refresh();return FReply::Handled();})
            [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [SNew(SShipSystemIcon).Icon(Group.IsNone()?EShipSystemIcon::Skills:CategoryIcon(Group))]
                +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6,0)
                [SNew(STextBlock).Text(Caption).Font(MissionUIStyle::GetRowFont(11)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]]];
    }
    auto List=SNew(SListView<FSkillItem>).ListItemsSource(&Table->Visible).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([Weak,Table](FSkillItem Item,const TSharedRef<STableViewBase>& Owner)
        {
            return SNew(SSkillRow,Owner).Item(Item).Service(Weak)
                .OnRequestTraining(FRequestSkillTraining::CreateLambda([Weak,Table](FName Skill,int32 Level)
                {
                    const auto* Found=Table->All.FindByPredicate([Skill](const FSkillItem& Entry){return Entry->Name==Skill;});
                    if(!Found || !Weak.IsValid())return;
                    Table->Selected=*Found;
                    if(Weak->GetLevel(Skill)>=Level)
                    {Table->PendingLevel=0;Table->Feedback=INVTEXT("This skill level is already trained.");}
                    else {Table->PendingLevel=Level;Table->Feedback=FText::GetEmpty();}
                    Table->Refresh();
                }));
        })
        .OnSelectionChanged_Lambda([Table](FSkillItem Item,ESelectInfo::Type) { Table->Selected=Item; Table->Feedback=FText::GetEmpty(); });
    Table->List=List;
    return SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(SSkillQueue).Service(Weak)]
        +SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SBorder).Visibility_Lambda([Table](){return Table->PendingLevel>0?EVisibility::Visible:EVisibility::Collapsed;})
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([Table](){return FText::Format(INVTEXT("TRAIN {0} TO LEVEL {1}? Each missing level will be queued separately."),Table->Selected?FText::FromString(Table->Selected->DisplayName):FText::GetEmpty(),FText::AsNumber(Table->PendingLevel));})]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Confirm training"))
                    .OnClicked_Lambda([Table,Weak](){if(Table->Selected && Weak.IsValid() && Weak->QueueToLevel(Table->Selected->Name,Table->PendingLevel,Table->Feedback,[Table](bool,const FText& Why){Table->Feedback=Why;Table->Refresh();}))Table->PendingLevel=0;Table->Refresh();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Cancel request")).OnClicked_Lambda([Table](){Table->PendingLevel=0;return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]]]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SSearchBox).HintText(INVTEXT("Search skills"))
            .OnTextChanged_Lambda([Table](const FText& Text) { Table->Search = Text.ToString().TrimStartAndEnd(); Table->Refresh(); })]
        + SVerticalBox::Slot().AutoHeight().Padding(8,2)[GroupButtons]
        + SVerticalBox::Slot().FillHeight(0.65f).Padding(8)
        [MissionUIStyle::MakeDataPane(List)]
        + SVerticalBox::Slot().FillHeight(0.35f).Padding(8)
        [MissionUIStyle::MakeDataPane(SNew(SScrollBox)+SScrollBox::Slot()[SNew(SRecordDetails).EventDriven(true).SelectionRevision_Lambda([Table](){return PointerHash(Table->Selected.Get());}).Record_Lambda([Table,Weak]()
            {
                FRecordDetails D;D.Icon=EShipSystemIcon::Skills;
                if(!Table->Selected){D.Title=INVTEXT("SELECT A SKILL BOOK");D.Description=INVTEXT("Standard books unlock Levels I-V. Advanced books unlock VI-X. Click a level dot to queue training; each level has its own saved GMT finish time.");return D;}
                const auto& Skill=*Table->Selected;FText Reason;const bool Allowed=Weak.IsValid() && Weak->CanPurchaseBook(Skill.Name,Reason);
                D.Title=FText::FromString(Skill.DisplayName);D.Subtitle=FText::FromName(Skill.Group);D.Description=FText::FromString(Skill.Description);
                D.Status=Weak.IsValid()?Weak->TrainingStatus(Skill.Name):INVTEXT("UNAVAILABLE");
                D.Fact(INVTEXT("STANDARD BOOK"),FText::Format(INVTEXT("{0} CR / I-V"),FText::AsNumber(Skill.BookPrice))).Fact(INVTEXT("ADVANCED BOOK"),FText::Format(INVTEXT("{0} CR / VI-X"),FText::AsNumber(Skill.AdvancedBookPrice)));
                TArray<FString> Required;for(FName Id:Skill.Prerequisites)if(Weak.IsValid()){const auto* Parent=Weak->GetCatalog().Find(Id);Required.Add(Parent?Parent->DisplayName:Id.ToString());}
                D.Fact(INVTEXT("MINIMUM RANK"),FText::FromString(UTF8_TO_TCHAR(PlayerCharacter::RankName(Skill.MinimumRank))));
                D.Fact(INVTEXT("PREREQUISITES"),Required.IsEmpty()?INVTEXT("NONE"):FText::FromString(FString::Join(Required,TEXT(", "))));
                D.Section(INVTEXT("PURCHASE STATUS"),Reason);return D;
            })])]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Table]() { return Table->Feedback; })]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("Purchase the standard skill book to unlock training for Levels I-V"))
                .IsEnabled_Lambda([Table,Weak]() { FText Reason; return Table->Selected && Weak.IsValid() && Weak->CanPurchaseBook(Table->Selected->Name,Reason); })
                .OnClicked_Lambda([Table,Weak]() { if (Table->Selected && Weak.IsValid()) Weak->PurchaseBook(Table->Selected->Name,Table->Feedback,[Table](bool,const FText& Why){Table->Feedback=Why;Table->Refresh();}); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Skills)]]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)
            [SNew(SButton).ToolTipText_Lambda([Table,Weak](){FText R;if(Table->Selected && Weak.IsValid())Weak->CanPurchaseAdvancedBook(Table->Selected->Name,R);return R;})
                .IsEnabled_Lambda([Table,Weak](){FText R;return Table->Selected && Weak.IsValid() && Weak->CanPurchaseAdvancedBook(Table->Selected->Name,R);})
                .OnClicked_Lambda([Table,Weak](){if(Table->Selected && Weak.IsValid())Weak->PurchaseAdvancedBook(Table->Selected->Name,Table->Feedback,[Table](bool,const FText& Why){Table->Feedback=Why;Table->Refresh();});Table->Refresh();return FReply::Handled();})
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]]];
}
