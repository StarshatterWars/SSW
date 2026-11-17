#include "SkillsDlg.h"
#include "PlayerCharacter.h"
#include "Widgets/Views/SExpanderArrow.h"
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
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
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
using FSkillItem = TSharedPtr<FSkillDefinition>;
class SSkillRow : public SMultiColumnTableRow<FSkillItem>
{
public:
    SLATE_BEGIN_ARGS(SSkillRow) {} SLATE_ARGUMENT(FSkillItem, Item) SLATE_ARGUMENT(TWeakObjectPtr<USkillsSubsystem>, Service) SLATE_END_ARGS()
    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    { Item = Args._Item; Service = Args._Service; SMultiColumnTableRow<FSkillItem>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner); }
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        if (Column==TEXT("Group")) return SNew(SBox).Padding(6).ToolTipText(FText::FromName(Item->Group))[SNew(SShipSystemIcon).Icon(CategoryIcon(Item->Group))];
        if (Column==TEXT("Status")) return SNew(SBox).Padding(6)
            .ToolTipText_Lambda([Item=Item,Weak=Service]() { if (Weak.IsValid() && Weak->HasSkill(Item->Name)) return INVTEXT("Skill complete"); FText Reason; if (Weak.IsValid()) Weak->CanLearn(Item->Name,Reason); return Reason; })
            [SNew(SWidgetSwitcher).WidgetIndex_Lambda([Item=Item,Weak=Service]() { if (Weak.IsValid() && Weak->HasSkill(Item->Name)) return 0; FText Reason; return Weak.IsValid() && Weak->CanLearn(Item->Name,Reason) ? 1 : 2; })
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]
                + SWidgetSwitcher::Slot()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]];
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(6,4))
            .Text_Lambda([Item = Item, Weak = Service, Column]()
            {
                if (Column == TEXT("Group")) return FText::FromName(Item->Group).ToUpper();
                if (Column == TEXT("Cost")) return FText::Format(INVTEXT("{0} CR"), FText::AsNumber(Item->BookPrice));
                if (Column == TEXT("Status")) return Weak.IsValid() && Weak->HasSkill(Item->Name) ? INVTEXT("COMPLETE") : INVTEXT("NOT LEARNED");
                return FText::FromString(Item->DisplayName).ToUpper();
            });
    }
private: FSkillItem Item; TWeakObjectPtr<USkillsSubsystem> Service;
};
struct FSkillTreeNode
{
    FName Category;
    FSkillItem Skill;
    TArray<TSharedPtr<FSkillTreeNode>> Children;
};
struct FSkillsTable
{
    TArray<TSharedPtr<FSkillTreeNode>> Roots;
    TArray<FSkillItem> All, Visible;
    TWeakPtr<SListView<FSkillItem>> List;
    FSkillItem Selected;
    FName SelectedGroup;
    TWeakObjectPtr<USkillsSubsystem> Service;
    FString Search;
    FName SortColumn = TEXT("Group");
    EColumnSortMode::Type SortMode = EColumnSortMode::Ascending;
    FText Feedback;
    void Refresh()
    {
        Visible.Reset();
        for (const auto& Item : All)
            if ((SelectedGroup.IsNone() || Search==TEXT("*") || Item->Group==SelectedGroup) && (Search.IsEmpty() || Search==TEXT("*") || Item->DisplayName.Contains(Search) || Item->Description.Contains(Search) || Item->Group.ToString().Contains(Search))) Visible.Add(Item);
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
    for (const auto& Column : {TPair<FName,FString>(TEXT("Group"),TEXT("GROUP")), {TEXT("Name"),TEXT("SKILL")}, {TEXT("Status"),TEXT("STATUS")}, {TEXT("Cost"),TEXT("BOOK PRICE")}})
        Header->AddColumn(SHeaderRow::Column(Column.Key).DefaultLabel(FText::FromString(Column.Value)).FillWidth(Column.Key == TEXT("Name") ? 2.f : 1.f)
            .SortMode_Lambda([Table, Id = Column.Key]() { return Table->SortColumn == Id ? Table->SortMode : EColumnSortMode::None; })
            .OnSort_Lambda([Table](EColumnSortPriority::Type, const FName& Id, EColumnSortMode::Type Mode) { Table->SortColumn = Id; Table->SortMode = Mode; Table->Refresh(); }));
    TMap<FName,TSharedPtr<FSkillTreeNode>> Categories,Nodes;
    for (const auto& Skill : Table->All)
    {
        auto Node=MakeShared<FSkillTreeNode>(); Node->Skill=Skill; Nodes.Add(Skill->Name,Node);
        if (!Categories.Contains(Skill->Group)) { auto Group=MakeShared<FSkillTreeNode>(); Group->Category=Skill->Group; Categories.Add(Skill->Group,Group); Table->Roots.Add(Group); }
    }
    for (const auto& Skill : Table->All)
    {
        auto Parent=Categories.FindChecked(Skill->Group);
        for (FName Required : Skill->Prerequisites)
            if (const auto* Candidate=Nodes.Find(Required); Candidate && (*Candidate)->Skill->Group==Skill->Group) { Parent=*Candidate; break; }
        Parent->Children.Add(Nodes.FindChecked(Skill->Name));
    }
    Table->Roots.Sort([](const auto& A,const auto& B) { return A->Category.LexicalLess(B->Category); });
    auto Tree=SNew(STreeView<TSharedPtr<FSkillTreeNode>>).TreeItemsSource(&Table->Roots).SelectionMode(ESelectionMode::Single)
        .OnGetChildren_Lambda([](TSharedPtr<FSkillTreeNode> Node,TArray<TSharedPtr<FSkillTreeNode>>& Children) { Children=Node->Children; })
        .OnGenerateRow_Lambda([Weak](TSharedPtr<FSkillTreeNode> Node,const TSharedRef<STableViewBase>& Owner)
        {
            auto Row=SNew(STableRow<TSharedPtr<FSkillTreeNode>>,Owner).Style(&MissionUIStyle::FlightTableRowStyle());
            Row->SetContent(SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[SNew(SExpanderArrow,Row)]
                + SHorizontalBox::Slot().AutoWidth().Padding(3)[SNew(SShipSystemIcon).Icon(Node->Skill ? EShipSystemIcon::Skills : CategoryIcon(Node->Category))]
                + SHorizontalBox::Slot().FillWidth(1)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(4,6))
                    .Text(FText::FromString(Node->Skill ? Node->Skill->DisplayName : Node->Category.ToString()).ToUpper())]);
            return Row;
        })
        .OnSelectionChanged_Lambda([Table](TSharedPtr<FSkillTreeNode> Node,ESelectInfo::Type)
        { if (Node) { Table->SelectedGroup=Node->Skill ? Node->Skill->Group : Node->Category; Table->Selected=Node->Skill; Table->Refresh(); } });
    for (const auto& Root : Table->Roots) Tree->SetItemExpansion(Root,true);
    auto List=SNew(SListView<FSkillItem>).ListItemsSource(&Table->Visible).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([Weak](FSkillItem Item,const TSharedRef<STableViewBase>& Owner) { return SNew(SSkillRow,Owner).Item(Item).Service(Weak); })
        .OnSelectionChanged_Lambda([Table](FSkillItem Item,ESelectInfo::Type) { Table->Selected=Item; Table->Feedback=FText::GetEmpty(); });
    Table->List=List;
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SSearchBox).HintText(INVTEXT("Search skills"))
            .OnTextChanged_Lambda([Table](const FText& Text) { Table->Search = Text.ToString().TrimStartAndEnd(); Table->Refresh(); })]
        + SVerticalBox::Slot().FillHeight(0.65f).Padding(8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[MissionUIStyle::MakeDataPane(Tree)]
            + SHorizontalBox::Slot().FillWidth(2)[MissionUIStyle::MakeDataPane(List)]]
        + SVerticalBox::Slot().FillHeight(0.35f).Padding(8)
        [MissionUIStyle::MakeDataPane(SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Margin(10).AutoWrapText(true)
            .Text_Lambda([Table,Weak]()
            {
                if (!Table->Selected) return INVTEXT("Select a skill book. Buying a book grants the complete skill immediately. Timed training is inactive.");
                const auto& Skill = *Table->Selected; FText Reason;
                if (Weak.IsValid()) Weak->CanPurchaseBook(Skill.Name,Reason);
                TArray<FString> Required; for (FName Id : Skill.Prerequisites)
                    if (Weak.IsValid()) { const auto* Parent = Weak->GetCatalog().Find(Id); Required.Add(Parent ? Parent->DisplayName : Id.ToString()); }
                return FText::Format(INVTEXT("{0}\n{1}\n\nPREREQUISITES: {2}\nMINIMUM RANK: {4}\n{3}"), FText::FromString(Skill.DisplayName).ToUpper(),
                    FText::FromString(Skill.Description), Required.IsEmpty() ? INVTEXT("NONE") : FText::FromString(FString::Join(Required,TEXT(", "))),Reason,FText::FromString(UTF8_TO_TCHAR(PlayerCharacter::RankName(Skill.MinimumRank))));
            }))]
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Table]() { return Table->Feedback; })]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("Purchase skill book and learn the complete skill"))
                .IsEnabled_Lambda([Table,Weak]() { FText Reason; return Table->Selected && Weak.IsValid() && Weak->CanPurchaseBook(Table->Selected->Name,Reason); })
                .OnClicked_Lambda([Table,Weak]() { if (Table->Selected && Weak.IsValid()) Weak->PurchaseBook(Table->Selected->Name,Table->Feedback); Table->Refresh(); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Skills)]]];
}
