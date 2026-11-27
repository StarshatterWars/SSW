#include "ShipStoreDlg.h"
#include "OwnedCommandAI.h"
#include "SimDataEvents.h"
#include "ShipStoreSubsystem.h"
#include "ShipSkinCatalog.h"
#include "ShipBriefingPreview.h"
#include "ShipDesignRegistry.h"
#include "ShipInsuranceSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterShipDesignSubsystem.h"
#include "SkillsSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "PlayerCharacter.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace
{
struct FStoreEntry
{
    FShipStoreOffer Offer;
    FGuid HullId;
    FString Empire;
    int32 RequiredRank=0,Owned=0;
};
using FStoreRow=TSharedPtr<FStoreEntry>;
struct FStoreView : TSharedFromThis<FStoreView>
{
    TWeakObjectPtr<UShipStoreSubsystem> Store;
    TWeakObjectPtr<UShipInsuranceSubsystem> Sales;
    TWeakObjectPtr<UStarshatterPlayerSubsystem> Players;
    TWeakObjectPtr<UShipBriefingPreview> Preview, ExpandedPreview;
    bool bExpanded = false;
    TArray<FShipSkinCatalogEntry> Skins;
    TArray<FStoreRow> All,Visible,HullTemplates;
    FDelegateHandle DataChanged;
    ~FStoreView(){if(DataChanged.IsValid())FSimDataEvents::Changed().Remove(DataChanged);}
    TWeakPtr<SListView<FStoreRow>> List;
    FStoreRow Selected;
    EShipStoreCategory Tab=EShipStoreCategory::Hulls;
    FString Search;
    FText Feedback;
    FName SortColumn="Item";
    EColumnSortMode::Type SortMode=EColumnSortMode::Ascending;
    int32 Quantity=1;
    int32 RentalDays=1;
    FGuid PendingRental;
    bool CanRent(FText& Why) const {return Selected && Tab==EShipStoreCategory::Hulls && Sales.IsValid() && Sales->CanRentHull(Selected->Offer.Design,Selected->HullId,RentalDays,Why);}
    void Rent(){FText Why;if(!CanRent(Why)){Feedback=Why;return;}if(PendingRental!=Selected->HullId){PendingRental=Selected->HullId;Feedback=FText::Format(INVTEXT("Click RENT again to confirm. {0}"),Why);return;}FOwnedActionSelection W;W.Action=EOwnedCommand::Rent;W.Design=Selected->Offer.Design;W.Hull=Selected->HullId;W.Days=RentalDays;Sales->RequestOwnedAction(W,[Weak=TWeakPtr<FStoreView>(AsShared())](bool,const FText& Why,FGuid){if(auto V=Weak.Pin()){V->Feedback=Why;V->Refresh();}},Feedback);PendingRental.Invalidate();Refresh();}
    int32 BuyQuantity() const {return Tab==EShipStoreCategory::Hulls || Tab==EShipStoreCategory::Skins?1:Quantity;}
    void Refresh()
    {
        Visible.Reset();
        const FGuid PreviouslySelected=Selected && Selected->Offer.Category==EShipStoreCategory::Hulls?Selected->HullId:FGuid();
        if(Sales.IsValid())
        {
            All.RemoveAll([](const auto& Row){return Row->Offer.Category==EShipStoreCategory::Hulls;});
            for(const auto& Template:HullTemplates)for(const FGuid Id:Sales->HullOffers(Template->Offer.Design))
            {auto Row=MakeShared<FStoreEntry>(*Template);Row->HullId=Id;Row->Offer.Price=Sales->HullPurchasePrice(Row->Offer.Design,Id);All.Add(Row);if(Id==PreviouslySelected)Selected=Row;}
        }
        TMap<FName,TArray<FGuid>> AvailableHulls;
        for(const auto& Row:All)
        {
            if(Store.IsValid())Row->Owned=Store->Stored(Row->Offer.Id);
            if(Row->Offer.Category==EShipStoreCategory::Skins && Players.IsValid())
                Row->Owned=Players->GetPlayerInfo().OwnedShipSkins.Contains(Row->Offer.Id)?1:0;
            if(Row->HullId.IsValid() && Sales.IsValid())
            {
                auto* Offers=AvailableHulls.Find(Row->Offer.Design);
                if(!Offers)Offers=&AvailableHulls.Add(Row->Offer.Design,Sales->HullOffers(Row->Offer.Design));
                if(!Offers->Contains(Row->HullId))continue;
            }
            if(Row->Offer.Category==Tab && (Search.IsEmpty() || Row->Offer.Name.Contains(Search) || Row->Offer.Group.Contains(Search) || Row->HullId.ToString().Contains(Search) || Row->HullId.ToString(EGuidFormats::DigitsWithHyphens).Contains(Search)))Visible.Add(Row);
        }
        Visible.StableSort([this](const auto& Left,const auto& Right)
        {
            const auto& A=*(SortMode==EColumnSortMode::Descending?Right:Left);
            const auto& B=*(SortMode==EColumnSortMode::Descending?Left:Right);
            if((SortColumn=="Price" || SortColumn=="Total" || SortColumn=="Charges") && A.Offer.Price!=B.Offer.Price)return A.Offer.Price<B.Offer.Price;
            if(SortColumn=="HullId")return A.HullId.ToString()<B.HullId.ToString();
            if(SortColumn=="Owned" && A.Owned!=B.Owned)return A.Owned<B.Owned;
            if(SortColumn=="Grade" && A.Offer.Grade!=B.Offer.Grade)return A.Offer.Grade<B.Offer.Grade;
            if(SortColumn=="Group" && A.Offer.Group!=B.Offer.Group)return A.Offer.Group.Compare(B.Offer.Group,ESearchCase::IgnoreCase)<0;
            return A.Offer.Name.Compare(B.Offer.Name,ESearchCase::IgnoreCase)<0;
        });
        if(Selected && !Visible.Contains(Selected)){Selected.Reset();UpdatePreview();}
        if(const auto Widget=List.Pin())Widget->RequestListRefresh();
    }
    void UpdatePreview()
    {
        if(!Preview.IsValid())return;
        const auto* Skin=Selected && Tab==EShipStoreCategory::Skins?Skins.FindByPredicate([this](const auto& Entry){return Entry.Id==Selected->Offer.Id;}):nullptr;
        const auto* Hull=Selected && (Tab==EShipStoreCategory::Hulls || Skin)?ShipDesignRegistry::Find(Selected->Offer.Design):nullptr;
        Preview->SetShipDesign(Hull,{},Skin?ShipSkinCatalog::Resolve(*Skin):nullptr);
        Preview->ResetView();
        if (auto* Large = ExpandedPreview.Get())
        { Large->SetShipDesign(Hull, {}, Skin ? ShipSkinCatalog::Resolve(*Skin) : nullptr); Large->ResetView(); }
    }
    bool CanBuy(FText& Reason) const
    {
        if(Store.IsValid() && Store->IsPurchasePending()){Reason=INVTEXT("PURCHASE PROCESSING - PLEASE WAIT");return false;}
        if(!Selected){Reason=INVTEXT("Select an item to review its quote.");return false;}
        if(Tab==EShipStoreCategory::Hulls)
        {if(Sales.IsValid())return Selected->HullId.IsValid() && Sales->CanPurchaseHull(Selected->Offer.Design,Reason,Selected->HullId);return false;}
        return Store.IsValid() && Store->CanPurchase(Selected->Offer.Id,BuyQuantity(),Reason);
    }
    void Buy()
    {
        FText Reason;if(!CanBuy(Reason)){Feedback=Reason;return;}
        if(Tab==EShipStoreCategory::Hulls)
        {FOwnedActionSelection W;W.Action=EOwnedCommand::Purchase;W.Design=Selected->Offer.Design;W.Hull=Selected->HullId;Sales->RequestOwnedAction(W,[Weak=TWeakPtr<FStoreView>(AsShared())](bool,const FText& Why,FGuid){if(auto V=Weak.Pin()){V->Feedback=Why;V->Refresh();}},Feedback);}
        else
        {
            const auto Weak=TWeakPtr<FStoreView>(AsShared());
            Store->RequestPurchase(Selected->Offer.Id,BuyQuantity(),Selected->Offer.Price*BuyQuantity(),[Weak](bool,const FText& Message)
            {if(const auto View=Weak.Pin()){View->Feedback=Message;View->Refresh();}},Feedback);
        }
        Refresh();
    }
};
class SStoreRow : public SMultiColumnTableRow<FStoreRow>
{
public:
    SLATE_BEGIN_ARGS(SStoreRow){} SLATE_ARGUMENT(FStoreRow,Entry) SLATE_END_ARGS()
    void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
    {Entry=Args._Entry;SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner);}
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        FText Label;
        if(Column=="Item")Label=FText::FromString(Entry->Offer.Name);
        else if(Column=="HullId")Label=Entry->HullId.IsValid()?FText::FromString(Entry->HullId.ToString(EGuidFormats::DigitsWithHyphens)):INVTEXT("-");
        else if(Column=="Group")Label=FText::FromString(Entry->Offer.Group);
        else if(Column=="Grade")Label=Entry->Offer.Grade>0?FText::FromString(FString::FromInt(Entry->Offer.Grade)):INVTEXT("—");
        else if(Column=="Price")Label=FText::AsNumber(CommerceCharges::Quote(Entry->Offer.Price,ECommercePurpose::Purchase).Base);
        else if(Column=="Charges"){const auto Quote=CommerceCharges::Quote(Entry->Offer.Price,ECommercePurpose::Purchase);Label=FText::AsNumber(Quote.Tax+Quote.Fee);}
        else if(Column=="Total")Label=FText::AsNumber(Entry->Offer.Price);
        else if(Column=="Owned")Label=FText::AsNumber(Entry->Owned);
        else if(Column=="Access")Label=FText::FromString(Entry->Offer.Access);
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Margin(FMargin(8,5)).Text(Label.ToUpper())
            .ToolTipText(Column=="Charges"?CommerceCharges::Quote(Entry->Offer.Price,ECommercePurpose::Purchase).Breakdown():Label).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
            .Justification(Column=="Price" || Column=="Owned"?ETextJustify::Right:ETextJustify::Left);
    }
private:FStoreRow Entry;
};
}

TSharedRef<SWidget> UShipStoreDlg::RebuildWidget()
{
    const auto View=MakeShared<FStoreView>();auto* Game=GetGameInstance();
    if(Game)
    {
        View->Store=Game->GetSubsystem<UShipStoreSubsystem>();View->Sales=Game->GetSubsystem<UShipInsuranceSubsystem>();
        View->Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        if(View->Players.IsValid() && View->Players->HasLoaded())View->Players->GetEconomy();
        if(View->Sales.IsValid())View->Sales->PrepareCarrierFighterSales(View->Feedback);
        if(View->Store.IsValid() && View->Store->RefreshCatalog(View->Feedback))
            for(const auto& Offer:View->Store->GetOffers()){auto Row=MakeShared<FStoreEntry>();Row->Offer=Offer;View->All.Add(Row);}
        const auto* Catalog=Game->GetSubsystem<UStarshatterShipDesignSubsystem>();
        const auto* Pilot=PlayerCharacter::GetCurrentPlayer();const int32 Rank=Pilot?Pilot->GetRank():0;
        const auto* Empire=StaticEnum<EShipEmpire>();
        const int64 PilotEmpire=Pilot?Empire->GetValueByNameString(StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(int64(Pilot->GetEmpire()))):INDEX_NONE;
        TArray<FName> StockDesigns;
        if(Catalog)for(const auto& Pair:Catalog->GetDesignsByName())StockDesigns.Add(FName(*Pair.Value.ShipName));
        const bool OffersReady=View->Sales.IsValid() && View->Sales->PrepareHullOffers(StockDesigns,View->Feedback);
        if(Catalog)for(const auto& Pair:Catalog->GetDesignsByName())
        {
            const auto& Hull=Pair.Value;
            if(Hull.Hidden || Hull.BaseCost<=0 || (Hull.Category!=EShipCategory::CapitalShip && Hull.Category!=EShipCategory::Fighter &&
                Hull.Category!=EShipCategory::Shuttle && Hull.Category!=EShipCategory::Transport))continue;
            const int32 Required=PlayerCharacter::HullRankRequired(Hull.ShipType,Hull.ShipName);
            if(Rank<Required || (Hull.ShipEmpire!=EShipEmpire::All && Hull.ShipEmpire!=EShipEmpire::Neutral && int64(Hull.ShipEmpire)!=PilotEmpire))continue;
            auto Row=MakeShared<FStoreEntry>();Row->Offer.Category=EShipStoreCategory::Hulls;Row->Offer.Id=Pair.Key;
            Row->Offer.Design=FName(*Hull.ShipName);Row->Offer.Name=Hull.DisplayName.IsEmpty()?Hull.ShipName:Hull.DisplayName;
            Row->Offer.Group=Hull.ShipClass;Row->Offer.Description=Hull.Description;Row->RequiredRank=Required;
            Row->Offer.Access=ANSI_TO_TCHAR(PlayerCharacter::RankName(Required));
            const auto* Skills=Game->GetSubsystem<USkillsSubsystem>();const auto Skill=USkillsSubsystem::RequiredHullSkill(Hull.ShipType);
            if(!Skill.IsNone())Row->Offer.Access+=Skills && Skills->HasSkill(Skill)?TEXT(" / SKILL MET"):TEXT(" / SKILL REQUIRED");
            Row->Empire=Empire->GetDisplayNameTextByValue(int64(Hull.ShipEmpire)).ToString();
            Row->Offer.Price=View->Sales.IsValid()?View->Sales->HullPurchasePrice(Row->Offer.Design):Hull.BaseCost;
            View->HullTemplates.Add(Row);
            if(OffersReady)
                for(const FGuid Id:View->Sales->HullOffers(Row->Offer.Design))
                {auto Specific=MakeShared<FStoreEntry>(*Row);Specific->HullId=Id;Specific->Offer.Price=View->Sales->HullPurchasePrice(Row->Offer.Design,Id);View->All.Add(Specific);}
        }
    }
    FString SkinError;ShipSkinCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/skins.json"),View->Skins,SkinError);
    Preview=NewObject<UShipBriefingPreview>(this);Preview->SetPurchasePreview();Preview->SetVisibility(ESlateVisibility::Visible);Preview->SetToolTipText(INVTEXT("Drag to orbit / mouse wheel to zoom / double-click to reset"));View->Preview=Preview;
    ExpandedPreview.Reset(NewObject<UShipBriefingPreview>(this)); ExpandedPreview->SetPurchasePreview();
    ExpandedPreview->SetToolTipText(INVTEXT("Drag to rotate / wheel to zoom / double-click to reset"));
    View->ExpandedPreview=ExpandedPreview.Get();
    const auto Weak=TWeakPtr<FStoreView>(View);
    auto Header=SNew(SHeaderRow);
    const auto Column=[&](FName Id,FText Label,float Width)
    {
        Header->AddColumn(SHeaderRow::Column(Id).DefaultLabel(Label).FillWidth(Width)
            .SortMode_Lambda([Weak,Id](){const auto State=Weak.Pin();return State && State->SortColumn==Id?State->SortMode:EColumnSortMode::None;})
            .OnSort_Lambda([Weak](EColumnSortPriority::Type,const FName& Id,EColumnSortMode::Type Mode){if(auto State=Weak.Pin()){State->SortColumn=Id;State->SortMode=Mode;State->Refresh();}}));
    };
    Column("Item",INVTEXT("ITEM"),2);Column("HullId",INVTEXT("HULL ID"),1.4f);Column("Group",INVTEXT("TYPE / HULL"),1.2f);Column("Grade",INVTEXT("GRADE"),.55f);
    Column("Price",INVTEXT("BASE CR"),.8f);Column("Charges",INVTEXT("TAX / FEES"),.7f);Column("Total",INVTEXT("TOTAL CR"),.8f);Column("Owned",INVTEXT("AT DOCK"),.65f);Column("Access",INVTEXT("ACCESS / CATEGORY"),1.3f);
    auto List=SNew(SListView<FStoreRow>).ListItemsSource(&View->Visible).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([](FStoreRow Row,const TSharedRef<STableViewBase>& Owner)->TSharedRef<ITableRow>{return SNew(SStoreRow,Owner).Entry(Row);})
        .OnSelectionChanged_Lambda([Weak](FStoreRow Row,ESelectInfo::Type){if(auto State=Weak.Pin()){State->Selected=Row;State->PendingRental.Invalidate();State->UpdatePreview();}});
    View->List=List;View->Refresh();
    View->DataChanged=FSimDataEvents::Changed().AddLambda([Weak](const FEconomyState& State)
    {if(const auto V=Weak.Pin();V && V->Players.IsValid() && &V->Players->GetPlayerInfo().Economy==&State)V->Refresh();});
    auto RentalTerms=SNew(SHorizontalBox);
    for(const int32 Days:{1,3,7})RentalTerms->AddSlot().FillWidth(1).Padding(2,0)[SNew(SButton)
        .ButtonColorAndOpacity_Lambda([View,Days](){return View->RentalDays==Days?MissionUIStyle::RowSelectedBG:MissionUIStyle::RowBG;})
        .OnClicked_Lambda([View,Days](){View->RentalDays=Days;View->PendingRental.Invalidate();return FReply::Handled();})
        [SNew(STextBlock).Text(FText::Format(INVTEXT("{0}D"),FText::AsNumber(Days))).Font(MissionUIStyle::GetRowFont(11))]];
    auto Tabs=SNew(SHorizontalBox);
    const TCHAR* Names[]={TEXT("HULLS"),TEXT("SKINS"),TEXT("COMPONENTS"),TEXT("MISSILES"),TEXT("DRONES")};
    const EShipSystemIcon Icons[]={EShipSystemIcon::Ship,EShipSystemIcon::Appearance,EShipSystemIcon::Reactor,EShipSystemIcon::MediumRangeMissile,EShipSystemIcon::MiningDrone};
    for(int32 Index=0;Index<5;++Index)
        Tabs->AddSlot().FillWidth(1).Padding(2,0)[SNew(SButton).ToolTipText(FText::FromString(Names[Index]))
            .ButtonColorAndOpacity_Lambda([View,Index](){return int32(View->Tab)==Index?MissionUIStyle::RowSelectedBG:MissionUIStyle::RowBG;})
            .OnClicked_Lambda([View,Index](){View->Tab=EShipStoreCategory(Index);View->Selected.Reset();View->Refresh();View->UpdatePreview();return FReply::Handled();})
            [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth()[SNew(SShipSystemIcon).Icon(Icons[Index])]
                +SHorizontalBox::Slot().FillWidth(1).Padding(5,0)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text(FText::FromString(Names[Index]))]]];
    auto StoreContent = SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(8,5)[Tabs]
        +SVerticalBox::Slot().AutoHeight().Padding(8,5)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1)[SNew(SEditableTextBox).HintText(INVTEXT("SEARCH ITEMS"))
                .OnTextChanged_Lambda([View](const FText& Text){View->Search=Text.ToString();View->Refresh();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(12,0)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([View](){return FText::Format(INVTEXT("WALLET  {0} CR"),FText::AsNumber(View->Store.IsValid()?View->Store->Balance():0));})]]
        +SVerticalBox::Slot().FillHeight(.45f).Padding(8,4)[MissionUIStyle::MakeDataPane(List,FMargin(0))]
        +SVerticalBox::Slot().FillHeight(.55f).Padding(8,4)[SNew(SBox)
            [SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(2)[MissionUIStyle::MakeDataPane(SNew(SScrollBox)+SScrollBox::Slot()[SNew(STextBlock)
                .Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([View]()
                {
                    if(!View->Selected)return INVTEXT("SELECT AN ITEM\nHulls retain rank, skill, empire and local-stock restrictions. Equipment is delivered to this dock, not automatically equipped.");
                    const auto& Row=*View->Selected;
                    return FText::FromString(Row.Offer.Name.ToUpper()+(Row.HullId.IsValid()?TEXT("\nHULL ID: ")+Row.HullId.ToString(EGuidFormats::DigitsWithHyphens).ToUpper():FString())+TEXT("\n")+Row.Offer.Description+TEXT("\n")+
                        (Row.Empire.IsEmpty()?FString():TEXT("EMPIRE: ")+Row.Empire+TEXT("  |  "))+
                        TEXT("DELIVERY: ")+(View->Store.IsValid()?View->Store->Dock():TEXT("DOCK REQUIRED")));
                })])]
                +SHorizontalBox::Slot().FillWidth(1.4f).Padding(8,0,0,0)[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                    +SVerticalBox::Slot().FillHeight(1)[Preview->TakeWidget()]
                    +SVerticalBox::Slot().AutoHeight()[SNew(SButton).ToolTipText(INVTEXT("Open large ship and skin viewer"))
                        .IsEnabled_Lambda([View](){return View->Selected && (View->Tab==EShipStoreCategory::Hulls || View->Tab==EShipStoreCategory::Skins);})
                        .OnClicked_Lambda([View](){View->bExpanded=true;View->UpdatePreview();return FReply::Handled();})
                        [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Camera)]])]]]
        +SVerticalBox::Slot().AutoHeight().Padding(8,4)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(INVTEXT("QUANTITY")).Font(MissionUIStyle::GetRowFont(11))]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SBox).WidthOverride(100)[SNew(SSpinBox<int32>).MinValue(1).MaxValue(1000)
                .Value_Lambda([View](){return View->BuyQuantity();}).IsEnabled_Lambda([View](){return View->Tab!=EShipStoreCategory::Hulls && View->Tab!=EShipStoreCategory::Skins;})
                .OnValueChanged_Lambda([View](int32 Value){View->Quantity=FMath::Clamp(Value,1,1000);})]]
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true)
                .Text_Lambda([View](){FText Reason;View->CanBuy(Reason);
                    return View->Selected?FText::Format(INVTEXT("{0}\n{1}"),Reason,CommerceCharges::Quote(View->Selected->Offer.Price*View->BuyQuantity(),ECommercePurpose::Purchase).Breakdown()):Reason;})]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton)
                .Visibility_Lambda([View](){return View->Selected && View->Selected->Offer.Id==FName(TEXT("Drone/MiningDrone"))?EVisibility::Visible:EVisibility::Collapsed;})
                .ToolTipText(INVTEXT("Transfer purchased mining drones from this dock to cargo; then use Mining Drone Controls to load a bay"))
                .IsEnabled_Lambda([View](){FText Reason;return View->Store.IsValid() && View->Store->CanTransferMiningDrone(View->BuyQuantity(),Reason);})
                .OnClicked_Lambda([View](){if(View->Sales.IsValid()){FOwnedActionSelection W;W.Action=EOwnedCommand::TransferDrone;W.Quantity=View->BuyQuantity();View->Sales->RequestOwnedAction(W,[Weak=TWeakPtr<FStoreView>(View)](bool,const FText& Why,FGuid){if(auto V=Weak.Pin()){V->Feedback=Why;V->Refresh();}},View->Feedback);}View->Refresh();return FReply::Handled();})
                [SNew(STextBlock).Text(INVTEXT("TO CARGO")).Font(MissionUIStyle::GetRowFont(11))]]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton)
                .IsEnabled_Lambda([View](){FText Reason;return View->CanBuy(Reason);})
                .ToolTipText(INVTEXT("Purchase the quoted item and quantity with credits"))
                .OnClicked_Lambda([View](){View->Buy();return FReply::Handled();})
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Marketplace)]
                    +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[SNew(STextBlock).Text(INVTEXT("PURCHASE")).Font(MissionUIStyle::GetRowFont(12))]]]]
        +SVerticalBox::Slot().AutoHeight().Padding(8,4)[SNew(SHorizontalBox).Visibility_Lambda([View](){return View->Tab==EShipStoreCategory::Hulls?EVisibility::Visible:EVisibility::Collapsed;})
            +SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(INVTEXT("RENTAL TERM")).Font(MissionUIStyle::GetRowFont(11))]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SBox).WidthOverride(180)[RentalTerms]]
            +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(11)).Text_Lambda([View](){FText Why;View->CanRent(Why);return Why;})]
            +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).ToolTipText(INVTEXT("Rent this specific hull; confirm the term, mandatory insurance and liability before charging"))
                .IsEnabled_Lambda([View](){FText Why;return View->CanRent(Why);}).OnClicked_Lambda([View](){View->Rent();return FReply::Handled();})
                [SNew(STextBlock).Text_Lambda([View](){return View->PendingRental.IsValid()?INVTEXT("CONFIRM RENTAL"):INVTEXT("RENT");}).Font(MissionUIStyle::GetRowFont(12))]]]
        +SVerticalBox::Slot().AutoHeight().Padding(8,4)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).AutoWrapText(true).Text_Lambda([View](){return View->Feedback;})];
    return SNew(SOverlay)
        +SOverlay::Slot()[StoreContent]
        +SOverlay::Slot().Padding(12)
        [SNew(SBox).Visibility_Lambda([View](){return View->bExpanded?EVisibility::Visible:EVisibility::Collapsed;})
        [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(INVTEXT("SHIP VIEWER")).Font(MissionUIStyle::GetHeaderFont(16))]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Reset view"))
                    .OnClicked_Lambda([View](){if(auto* P=View->ExpandedPreview.Get())P->ResetView();return FReply::Handled();})
                    [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Refresh)]]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Close ship viewer"))
                    .OnClicked_Lambda([View](){View->bExpanded=false;return FReply::Handled();})
                    [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Abort)]]]
            +SVerticalBox::Slot().FillHeight(1)[ExpandedPreview->TakeWidget()])
        ]];
}
