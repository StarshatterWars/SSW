#include "ForcesDlg.h"
#include "CampaignFleet.h"
#include "ForcesKnowledge.h"
#include "SkillsSubsystem.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Sim.h"
#include "SimContact.h"
#include "Campaign.h"
#include "Mission.h"
#include "CombatGroup.h"
#include "FighterHUDDetails.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimElement.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
namespace
{
using FReport=TSharedPtr<FForcesReport>;
class SForceRow : public SMultiColumnTableRow<FReport>
{
public:
    SLATE_BEGIN_ARGS(SForceRow) {} SLATE_ARGUMENT(FReport,Item) SLATE_END_ARGS()
    void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
    { Item=Args._Item; SMultiColumnTableRow<FReport>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner); }
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        FString Value=Column==TEXT("Status") ? (Item->bAlliedOperationalAccess && Item->IFF==Item->ObserverIFF && Item->IFF>0?Item->Status:TEXT("—")) : Column==TEXT("HullId") ? (Item->bAlliedOperationalAccess && Item->IFF==Item->ObserverIFF && Item->IFF>0 && Item->HullId.IsValid()?Item->HullId.ToString(EGuidFormats::DigitsWithHyphens):TEXT("—")) : Column==TEXT("Design") ? Item->Design : Column==TEXT("System") ? Item->System : Column==TEXT("Sector") ? Item->Sector :
            Column==TEXT("Group") ? Item->Group : Column==TEXT("IFF") ? (Item->IFF==Item->ObserverIFF ? TEXT("FRIENDLY") : Item->IFF==0 ? TEXT("NEUTRAL") : TEXT("IFF: KNOWN")) : Column==TEXT("Hull") ? (Item->bAlliedOperationalAccess && Item->IFF==Item->ObserverIFF && Item->IFF>0?FString::Printf(TEXT("%.0f%%"),Item->Integrity*100):TEXT("—")) : Item->Name;
        return SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(6,4)).Text(FText::FromString(Value).ToUpper()).ColorAndOpacity(SFighterHUDDetails::IdentificationColor(Item->IFF,Item->ObserverIFF)).ToolTipText(FText::FromString(Value));
    }
private:FReport Item;
};
struct FForceTable
{
    TWeakObjectPtr<UGameInstance> Instance;
    TArray<FReport> Rows;
    TWeakPtr<SListView<FReport>> List;
    FReport Selected;
    EForcesScope Requested=EForcesScope::Sector;
    int32 SkillLevel=0;
    FString Location;
    FName SortColumn=TEXT("Name"); bool Descending=false;
    void Refresh()
    {
        auto* Simulation=Sim::GetSim(); auto* Player=Simulation ? Simulation->GetPlayerShip() : nullptr;
        auto* Region=Player ? Player->GetRegion() : nullptr; auto* System=Region ? Region->GetSystem() : nullptr;
        Rows.Reset();
        if (!Region || !System) { Selected.Reset(); if (auto View=List.Pin()) View->RequestListRefresh(); return; }
        auto* Skills=Instance.IsValid() ? Instance->GetSubsystem<USkillsSubsystem>() : nullptr;
        SkillLevel=Skills && Skills->HasSkill(TEXT("sensor_networking"),5) ? 5 : Skills && Skills->HasSkill(TEXT("sensor_networking"),3) ? 3 : Skills && Skills->HasSkill(TEXT("sensor_networking"),1) ? 1 : 0;
        if (SkillLevel<1)
        { Location=TEXT("SENSOR NETWORKING I REQUIRED"); Selected.Reset(); if(auto View=List.Pin())View->RequestListRefresh(); return; }
        const auto Scope=static_cast<EForcesScope>(FMath::Min(static_cast<uint8>(Requested),static_cast<uint8>(FForcesKnowledge::ScopeForLevel(SkillLevel))));
        TSet<FName> Neighbors; TSet<FGuid> LiveHulls;TArray<FForcesReport> Reports; TMap<const Ship*,int32> KnownIFF;
        for (ListIter<SimRegion> Regions=Simulation->GetRegions();++Regions;)
            if (Regions.value() && Regions->GetSystem() && System->HasLinkTo(Regions->GetSystem()))
                Neighbors.Add(FName(UTF8_TO_TCHAR(Regions->GetSystem()->GetName())));
        const auto ReadContacts=[&KnownIFF](Ship* Observer)
        {
            for (ListIter<SimContact> Contacts=Observer->GetContactList();++Contacts;)
                if (auto* Contact=Contacts.value(); Contact && Contact->GetShip() && Contact->GetIFF(Observer)>=0)
                    KnownIFF.Add(Contact->GetShip(),Contact->GetIFF(Observer));
        };
        ReadContacts(Player);
        if (Scope!=EForcesScope::Sector)
            for (ListIter<SimRegion> Regions=Simulation->GetRegions();++Regions;)
            {
                auto* Source=Regions.value(); if (!Source || !Source->GetSystem() ||
                    (Source->GetSystem()!=System && (Scope!=EForcesScope::Neighbors || !Neighbors.Contains(FName(UTF8_TO_TCHAR(Source->GetSystem()->GetName())))))) continue;
                for (ListIter<Ship> Observers=Source->GetShips();++Observers;)
                    if (auto* Observer=Observers.value();Observer && !Observer->IsDead() && Observer->GetIFF()==Player->GetIFF()) ReadContacts(Observer);
            }
        const auto* Mission=Simulation->GetMission();
        const bool FreeMode=Mission && Mission->IsFreeFlight();
        auto* ActiveCampaign=Campaign::GetCampaign();
        const bool CampaignGate=!FreeMode && ActiveCampaign && !ActiveCampaign->IsTraining() && ActiveCampaign->GetCampaignId()<Campaign::SINGLE_MISSIONS;
        for (ListIter<SimRegion> Regions=Simulation->GetRegions();++Regions;)
        {
            auto* Candidate=Regions.value(); if (!Candidate || !Candidate->GetSystem()) continue;
            if ((Scope==EForcesScope::Sector && Candidate!=Region) ||
                (Candidate->GetSystem()!=System && (Scope!=EForcesScope::Neighbors || !Neighbors.Contains(FName(UTF8_TO_TCHAR(Candidate->GetSystem()->GetName())))))) continue;
            for (ListIter<Ship> Ships=Candidate->GetShips();++Ships;)
            {
                const auto* Unit=Ships.value(); if (!Unit || Unit->IsDead() || Unit->IsDying()) continue;
                auto* Element=Unit->GetElement(); auto* Group=Element ? Element->GetCombatGroup() : nullptr;
                bool Authorized=!CampaignGate;
                if (CampaignGate && Group)
                {
                    Authorized=true;
                    for (auto* Parent=Group;Parent;Parent=Parent->GetParent()) if (Parent->GetIntelLevel()<EIntel::KNOWN) Authorized=false;
                }
                int32 IFF=Unit->GetIFF()==Player->GetIFF() ? Player->GetIFF() : KnownIFF.FindRef(Unit);
                const bool Reported=Unit->GetIFF()==Player->GetIFF() || KnownIFF.Contains(Unit) || (CampaignGate && Authorized);
                if (!Reported) continue;
                if (!KnownIFF.Contains(Unit) && Unit->GetIFF()!=Player->GetIFF()) IFF=Unit->GetIFF(); // Known campaign intelligence is an authorized report.
                FForcesReport Report; Report.Id=Unit->GetIdentity();Report.HullId=Unit->GetHullId();LiveHulls.Add(Report.HullId);
                Report.bAlliedOperationalAccess=Unit->GetIFF()==Player->GetIFF();
                Report.Status=Unit->GetIFF()==Player->GetIFF()?(FCampaignFleet::CanMove(*const_cast<Ship*>(Unit))?(Unit->Design() && Unit->GetIntegrity()<Unit->Design()->integrity?TEXT("DAMAGED"):TEXT("READY")):TEXT("IMMOBILIZED")):TEXT("REPORTED");
                if(ActiveCampaign)if(const auto* Saved=ActiveCampaign->GetPersistentFleet().Hulls.Find(Report.HullId);Saved && Saved->bRepairing && Unit->GetIFF()==Player->GetIFF())Report.Status=FCampaignFleet::Status(*Saved); Report.Name=UTF8_TO_TCHAR(Unit->GetObserverName());
                Report.Design=Unit->Design() ? UTF8_TO_TCHAR(Unit->Design()->name) : TEXT("");
                Report.System=UTF8_TO_TCHAR(Candidate->GetSystem()->GetName()); Report.Sector=UTF8_TO_TCHAR(Candidate->GetName());
                Report.Group=Group ? UTF8_TO_TCHAR(Group->GetDescription()) : Element ? UTF8_TO_TCHAR(Element->GetName()) : TEXT("INDEPENDENT UNIT");
                Report.Integrity=Unit->Design() && Unit->Design()->integrity>0 ? FMath::Clamp(Unit->GetIntegrity()/Unit->Design()->integrity,0.0,1.0) : 0; Report.Speed=Unit->GetVelocity().Size(); Report.IFF=IFF; Report.ObserverIFF=Player->GetIFF();
                Report.bReported=true; Report.bCampaignAllowed=Authorized; Reports.Add(Report);
            }
        }
        if(ActiveCampaign)
            for(const auto& Pair:ActiveCampaign->GetPersistentFleet().Hulls)
            {
                const auto& Hull=Pair.Value;
                if(Hull.Faction!=Player->GetIFF() || Hull.bTransferred || LiveHulls.Contains(Hull.HullId))continue;
                FForcesReport Report;Report.HullId=Hull.HullId;Report.Name=Hull.Name;Report.Design=Hull.Design.ToString();Report.Group=Hull.Assignment;
                Report.System=Hull.System;Report.Sector=Hull.Region;Report.Integrity=Hull.bDestroyed?0:Hull.Condition.bHasCondition?Hull.Condition.HullCondition:1;
                Report.bAlliedOperationalAccess=true;Report.Status=FCampaignFleet::Status(Hull);Report.IFF=Report.ObserverIFF=Player->GetIFF();Report.bReported=true;Report.bCampaignAllowed=true;Reports.Add(MoveTemp(Report));
            }
        const auto Visible=FForcesKnowledge::Filter(Reports,Player->GetIFF(),FName(UTF8_TO_TCHAR(System->GetName())),FName(UTF8_TO_TCHAR(Region->GetName())),NAME_None,Requested,SkillLevel,Neighbors);
        for (const auto& Report:Visible) Rows.Add(MakeShared<FForcesReport>(Report));
        Rows.StableSort([this](const FReport& A,const FReport& B)
        {
            int32 Order=SortColumn==TEXT("Status") ? A->Status.Compare(B->Status) : SortColumn==TEXT("HullId") ? A->HullId.ToString().Compare(B->HullId.ToString()) : SortColumn==TEXT("System") ? A->System.Compare(B->System) : SortColumn==TEXT("Sector") ? A->Sector.Compare(B->Sector) :
                SortColumn==TEXT("Design") ? A->Design.Compare(B->Design) : SortColumn==TEXT("Group") ? A->Group.Compare(B->Group) :
                SortColumn==TEXT("Hull") ? (A->Integrity<B->Integrity ? -1 : A->Integrity>B->Integrity ? 1 : 0) : 0;
            if (!Order) Order=A->Name.Compare(B->Name); return Descending ? Order>0 : Order<0;
        });
        Location=FString::Printf(TEXT("%hs / %hs  |  %d REPORTED UNITS"),System->GetName(),Region->GetName(),Rows.Num());
        if (Selected) { const uint32 Id=Selected->Id;const FGuid HullId=Selected->HullId; auto* Found=Rows.FindByPredicate([Id,HullId](const FReport& Item){return HullId.IsValid()?Item->HullId==HullId:Item->Id==Id;}); Selected=Found ? *Found : nullptr; }
        if (auto View=List.Pin()) View->RequestListRefresh();
    }
};
}
TSharedRef<SWidget> UForcesDlg::RebuildWidget()
{
    auto Model=MakeShared<FForceTable>(); Model->Instance=GetGameInstance(); Model->Refresh();
    auto Buttons=SNew(SHorizontalBox);
    const EShipSystemIcon Icons[]={EShipSystemIcon::Sensor,EShipSystemIcon::PlanetMoon,EShipSystemIcon::Jump};
    const TCHAR* Labels[]={TEXT("Sector forces â€” sensor level I"),TEXT("Current system forces â€” Sensor Networking III"),TEXT("Neighboring systems â€” Sensor Networking V")};
    for (int32 Index=0;Index<3;++Index)
        Buttons->AddSlot().AutoWidth().Padding(0,0,6,0)
        [SNew(SButton).ToolTipText(FText::FromString(Labels[Index]))
            .IsEnabled_Lambda([Model,Index](){return Model->SkillLevel>=(Index==0?1:Index==1?3:5);})
            .ButtonColorAndOpacity_Lambda([Model,Index](){return static_cast<int32>(Model->Requested)==Index ? MissionUIStyle::RowSelectedBG : MissionUIStyle::RowBG;})
            .OnClicked_Lambda([Model,Index](){Model->Requested=static_cast<EForcesScope>(Index);Model->Refresh();return FReply::Handled();})
            [SNew(SShipSystemIcon).Icon(Icons[Index])]];
    auto Header=SNew(SHeaderRow);
    for (const auto& Entry:{TPair<FName,FString>(TEXT("Name"),TEXT("UNIT")),{TEXT("Design"),TEXT("HULL")},{TEXT("Group"),TEXT("ELEMENT")},{TEXT("System"),TEXT("SYSTEM")},{TEXT("Sector"),TEXT("SECTOR")},{TEXT("IFF"),TEXT("IFF")},{TEXT("Hull"),TEXT("INTEGRITY")},{TEXT("Status"),TEXT("STATUS")},{TEXT("HullId"),TEXT("HULL ID")}})
        Header->AddColumn(SHeaderRow::Column(Entry.Key).DefaultLabel(FText::FromString(Entry.Value)).FillWidth(Entry.Key==TEXT("Name")?2.f:1.f)
            .OnSort_Lambda([Model](EColumnSortPriority::Type,const FName& Column,EColumnSortMode::Type Mode){Model->SortColumn=Column;Model->Descending=Mode==EColumnSortMode::Descending;Model->Refresh();}));
    auto List=SNew(SListView<FReport>).ListItemsSource(&Model->Rows).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
        .OnGenerateRow_Lambda([](FReport Item,const TSharedRef<STableViewBase>& Owner){return SNew(SForceRow,Owner).Item(Item);})
        .OnSelectionChanged_Lambda([Model](FReport Item,ESelectInfo::Type){Model->Selected=Item;});
    Model->List=List;
    List->RegisterActiveTimer(1.f,FWidgetActiveTimerDelegate::CreateLambda([Model](double,float){Model->Refresh();return EActiveTimerReturnType::Continue;}));
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)[Buttons]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Text_Lambda([Model](){return FText::FromString(Model->Location);})]
        + SVerticalBox::Slot().FillHeight(1).Padding(8)[MissionUIStyle::MakeDataPane(List)]
        + SVerticalBox::Slot().AutoHeight().Padding(8)[MissionUIStyle::MakeDataPane(SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Margin(10).AutoWrapText(true)
            .Text_Lambda([Model](){const auto& Item=Model->Selected; return Item ? FText::Format(INVTEXT("{0}  |  {1}\n{2} / {3}  |  {4}\nINTEGRITY {5}%  |  SPEED {6} M/S\nSTATUS {7}  |  HULL ID {8}"),
                FText::FromString(Item->Name).ToUpper(),FText::FromString(Item->Design).ToUpper(),FText::FromString(Item->System).ToUpper(),FText::FromString(Item->Sector).ToUpper(),
                FText::FromString(Item->Group).ToUpper(),Item->bAlliedOperationalAccess?FText::AsNumber(Item->Integrity*100):INVTEXT("—"),FText::AsNumber(Item->Speed),Item->bAlliedOperationalAccess?FText::FromString(Item->Status):INVTEXT("—"),Item->bAlliedOperationalAccess?FText::FromString(Item->HullId.ToString(EGuidFormats::DigitsWithHyphens)):INVTEXT("—")) : INVTEXT("SELECT A REPORTED UNIT. SYSTEM AND NEIGHBOR COVERAGE REQUIRE SENSOR NETWORKING III AND V.");}))];
}
