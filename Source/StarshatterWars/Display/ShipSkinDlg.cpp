#include "ShipSkinDlg.h"
#include "ShipSkinCatalog.h"
#include "StarshatterPlayerSubsystem.h"
#include "ShipInsuranceSubsystem.h"
#include "ShipBriefingPreview.h"
#include "ShipDesignRegistry.h"
#include "OwnedHullLocation.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "Misc/Paths.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
struct FSkinView : TSharedFromThis<FSkinView>
{
    TWeakObjectPtr<UStarshatterPlayerSubsystem> Players;
    TWeakObjectPtr<UShipBriefingPreview> Preview;
    TArray<FShipSkinCatalogEntry> Entries;
    TArray<TSharedPtr<FGuid>> Hulls;
    FGuid Hull;
    FName Selected;
    FString Search;
    FText Feedback;
    TSharedPtr<SVerticalBox> Rows;

    const FPlayerHangarShip* Craft() const
    {
        return Players.IsValid() ? Players->GetPlayerInfo().HangarShips.FindByPredicate(
            [&](const FPlayerHangarShip& Item){return Item.HullId==Hull;}) : nullptr;
    }
    const FShipSkinCatalogEntry* Selection() const
    {return Entries.FindByPredicate([&](const FShipSkinCatalogEntry& Entry){return Entry.Id==Selected;});}
    void UpdatePreview()
    {
        const auto* Entry=Selection();
        if(Preview.IsValid())Preview->SetShipDesign(Entry?ShipDesignRegistry::Find(Entry->Hull):Craft()?ShipDesignRegistry::Find(Craft()->Design):nullptr,{},Entry?ShipSkinCatalog::Resolve(*Entry):nullptr);
    }
    void Refresh()
    {
        if(!Rows || !Players.IsValid())return;
        Rows->ClearChildren();
        const auto& Pilot=Players->GetPlayerInfo();
        const auto* HullEntry=Craft();
        const TWeakPtr<FSkinView> Weak=AsShared();
        int32 Count=0;
        for(const auto& Entry:Entries)
        {
            if(!ShipSkinCatalog::IsPurchased(Pilot,Entry) || (HullEntry && HullEntry->Design!=Entry.Hull) ||
                (!Search.IsEmpty() && !Entry.Name.Contains(Search) && !Entry.Hull.ToString().Contains(Search)))continue;
            ++Count;
            const FName Id=Entry.Id;
            Rows->AddSlot().AutoHeight().Padding(0,2)
            [SNew(SButton).HAlign(HAlign_Fill).ContentPadding(FMargin(8,6))
                .ButtonColorAndOpacity_Lambda([Weak,Id](){const auto State=Weak.Pin();return State && State->Selected==Id?MissionUIStyle::RowSelectedBG:MissionUIStyle::RowBG;})
                .OnClicked_Lambda([Weak,Id](){if(auto State=Weak.Pin()){State->Selected=Id;State->UpdatePreview();}return FReply::Handled();})
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1.2f)[SNew(STextBlock).Text(FText::FromString(Entry.Name).ToUpper()).Font(MissionUIStyle::GetRowFont(11)).OverflowPolicy(ETextOverflowPolicy::Ellipsis).ToolTipText(FText::FromString(Entry.Name))]
                    +SHorizontalBox::Slot().FillWidth(.9f)[SNew(STextBlock).Text(FText::FromName(Entry.Hull).ToUpper()).Font(MissionUIStyle::GetRowFont(11)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
                    +SHorizontalBox::Slot().FillWidth(.8f)[SNew(STextBlock).Text(StaticEnum<EShipSkinCategory>()->GetDisplayNameTextByValue(int64(Entry.Category)).ToUpper()).Font(MissionUIStyle::GetRowFont(11))
                        .ToolTipText(StaticEnum<ESSWAsteroidRarity>()->GetDisplayNameTextByValue(int64(Entry.Rarity)))]
                    +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11))
                        .Text_Lambda([Weak,Id](){const auto State=Weak.Pin();const auto* Craft=State?State->Craft():nullptr;const auto* Entry=State?State->Entries.FindByPredicate([Id](const auto& Row){return Row.Id==Id;}):nullptr;return Craft && Craft->SelectedSkin==Id?INVTEXT("APPLIED"):State && Entry && State->Players.IsValid() && ShipSkinCatalog::IsOwned(State->Players->GetPlayerInfo(),*Entry)?INVTEXT("OWNED"):INVTEXT("AVAILABLE");})]]];
        }
        if(!Count)Rows->AddSlot().AutoHeight().Padding(8)
            [SNew(STextBlock).Text(INVTEXT("No owned skins match. Purchase licenses through Ship Store → Skins.")).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)];
    }
    FString HullName(FGuid Id) const
    {
        const auto* Item=Players.IsValid()?Players->GetPlayerInfo().HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Id;}):nullptr;
        return Item?Item->Name+TEXT(" / ")+Item->Design.ToString():TEXT("Browse purchased skins");
    }
    bool CanChange() const
    {
        const auto* HullEntry=Craft(); auto* Simulation=Sim::GetSim();const auto* Live=Simulation?Simulation->GetPlayerShip():nullptr;
        return HullEntry && !HullEntry->bAssigned && !HullEntry->bLost && Simulation && Simulation->HasSimulationAuthority() &&
            !Simulation->IsNetworked() && Live && Live->GetFlightPhase()<=EOPSMode::ALERT &&
            HullEntry->Location.Equals(OwnedHullLocation::Dock(*Live),ESearchCase::IgnoreCase);
    }
    void Apply(bool Reset)
    {
        if(!Players.IsValid() || !CanChange()){Feedback=INVTEXT("Dock at the base holding an owned craft to change its skin.");return;}
        const auto* Entry=Reset?nullptr:Selection();if(!Reset && !Entry)return;
        auto* Live=Sim::GetSim()->GetPlayerShip();
        const FS_PlayerGameInfo Before=Players->GetPlayerInfo();
        if(!ShipSkinCatalog::Apply(Players->GetMutablePlayerInfo(),Hull,Entry,OwnedHullLocation::Dock(*Live),Feedback))return;
        Players->MarkDirty();
        if(!Players->SaveActivePlayerProgress())
        {Players->GetMutablePlayerInfo()=Before;Feedback=INVTEXT("Could not save the appearance. Previous skin retained.");return;}
        if(Before.AssignedHangarHull==Hull && Live->Design())
            Live->UseSkin(Entry?Live->Design()->FindSkin(TCHAR_TO_UTF8(*Entry->SkinName)):nullptr);
        Refresh();
    }
};
}

TSharedRef<SWidget> UShipSkinDlg::RebuildWidget()
{
    auto* Instance=GetGameInstance();auto* Players=Instance?Instance->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
    if(!Players)return SNew(STextBlock).Text(INVTEXT("SHIP SKINS — AVAILABLE IN A LIVE PILOT SESSION")).Font(MissionUIStyle::GetHeaderFont(16));
    if(auto* Hangar=Instance->GetSubsystem<UShipInsuranceSubsystem>())Hangar->RefreshAssignment();
    Players->GetEconomy();
    auto State=MakeShared<FSkinView>();State->Players=Players;
    FString Error;
    if(!ShipSkinCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/skins.json"),State->Entries,Error))
        return MissionUIStyle::MakeDataPane(SNew(STextBlock).Text(FText::FromString(Error)).AutoWrapText(true));
    State->Hulls.Add(MakeShared<FGuid>()); // Browse the collection without owning every compatible hull.
    for(const auto& Hull:Players->GetPlayerInfo().HangarShips)
        if(!Hull.bLost && !Hull.bAssigned)State->Hulls.Add(MakeShared<FGuid>(Hull.HullId));
    if(!State->Hulls.IsEmpty())State->Hull=*State->Hulls[0];
    for(const auto& Entry:State->Entries)
        if(ShipSkinCatalog::IsPurchased(Players->GetPlayerInfo(),Entry) && (!State->Craft() || State->Craft()->Design==Entry.Hull))
        {State->Selected=Entry.Id;break;}
    Preview=NewObject<UShipBriefingPreview>(this);Preview->SetReferencePreview();State->Preview=Preview;
    State->Rows=SNew(SVerticalBox);State->Refresh();State->UpdatePreview();
    const auto Header=SNew(SHorizontalBox);
    for(const TCHAR* Caption:{TEXT("SKIN"),TEXT("HULL"),TEXT("CATEGORY"),TEXT("STATUS")})
        Header->AddSlot().FillWidth(1).Padding(8,5)[SNew(STextBlock).Text(FText::FromString(Caption)).Font(MissionUIStyle::GetRowFont(11))];
    return SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(STextBlock).Text(INVTEXT("PURCHASED SKINS")).Font(MissionUIStyle::GetRowFont(12))]
            +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)[SNew(SEditableTextBox).HintText(INVTEXT("Search purchased skins or hulls"))
                .OnTextChanged_Lambda([State](const FText& Text){State->Search=Text.ToString();State->Refresh();})]
            +SHorizontalBox::Slot().FillWidth(1)[SNew(SComboBox<TSharedPtr<FGuid>>).OptionsSource(&State->Hulls)
                .OnGenerateWidget_Lambda([State](TSharedPtr<FGuid> Id){return SNew(STextBlock).Text(FText::FromString(State->HullName(*Id)).ToUpper()).Font(MissionUIStyle::GetRowFont(12));})
                .OnSelectionChanged_Lambda([State](TSharedPtr<FGuid> Id,ESelectInfo::Type){if(Id){State->Hull=*Id;State->Selected=NAME_None;for(const auto& Entry:State->Entries)if(State->Craft() && Entry.Hull==State->Craft()->Design && ShipSkinCatalog::IsPurchased(State->Players->GetPlayerInfo(),Entry)){State->Selected=Entry.Id;break;}State->Refresh();State->UpdatePreview();}})
                [SNew(STextBlock).Text_Lambda([State](){return FText::FromString(State->HullName(State->Hull)).ToUpper();}).Font(MissionUIStyle::GetRowFont(12))]]]
        +SVerticalBox::Slot().FillHeight(1)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1.5f).Padding(0,0,8,0)
                [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Header]
                    +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[State->Rows.ToSharedRef()]])]
            +SHorizontalBox::Slot().FillWidth(1)[MissionUIStyle::MakeDataPane(Preview->TakeWidget())]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
        [MissionUIStyle::MakeDataPane(SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
            .Text_Lambda([State](){const auto* Entry=State->Selection();return Entry?FText::FromString(Entry->Name.ToUpper()+TEXT(" — ")+Entry->Description):INVTEXT("Select a purchased skin. Previewing never changes your craft.");}))]
        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
        [SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([State](){return State->Feedback;})]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).ToolTipText(INVTEXT("Apply selected skin — docked owned craft only"))
                .IsEnabled_Lambda([State](){return State->CanChange() && State->Selection();}).OnClicked_Lambda([State](){State->Apply(false);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Accept)]]
            +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Restore original hull materials"))
                .IsEnabled_Lambda([State](){return State->CanChange();}).OnClicked_Lambda([State](){State->Apply(true);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Refresh)]]];
}
