#include "GuildsDlg.h"
#include "GuildCatalogSubsystem.h"
#include "GuildAI.h"
#include "StationServices.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Engine/GameInstance.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
struct FGuildRow
{
    FName Definition;FGuid Id;bool bInvitation=false;
    FString Name,Kind,Empire,Status;
};
using FRow=TSharedPtr<FGuildRow>;
FString Roles(int32 Mask)
{
    TArray<FString> Names;
    for(const auto Role:{EGuildRole::Member,EGuildRole::Recruiter,EGuildRole::Quartermaster,EGuildRole::Treasurer,EGuildRole::Director,EGuildRole::Leader})
        if(Mask&static_cast<int32>(Role))Names.Add(StaticEnum<EGuildRole>()->GetNameStringByValue(static_cast<int64>(Role)));
    return FString::Join(Names,TEXT(", ")).ToUpper();
}
class SGuildRow:public SMultiColumnTableRow<FRow>
{
public:
    SLATE_BEGIN_ARGS(SGuildRow){}SLATE_ARGUMENT(FRow,Item)SLATE_END_ARGS()
    void Construct(const FArguments& Args,const TSharedRef<STableViewBase>& Owner)
    {Item=Args._Item;SMultiColumnTableRow<FRow>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()),Owner);}
    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        const FString Text=Column==TEXT("Name")?Item->Name:Column==TEXT("Kind")?Item->Kind:Column==TEXT("Empire")?Item->Empire:Item->Status;
        return SNew(STextBlock).Text(FText::FromString(Text.ToUpper())).ToolTipText(FText::FromString(Text)).Font(MissionUIStyle::GetRowFont(12)).Margin(FMargin(6,4)).OverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }
private:FRow Item;
};
class SGuildPanel:public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGuildPanel){}SLATE_ARGUMENT(UGameInstance*,Instance)SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Service=Args._Instance?Args._Instance->GetSubsystem<UGuildCatalogSubsystem>():nullptr;
        if(Service.IsValid() && FStationServices::IsDocked())Service->RegisterLocalPilot(Feedback);
        auto Tabs=SNew(SHorizontalBox);
        const EShipSystemIcon Icons[]={EShipSystemIcon::Skills,EShipSystemIcon::Guilds,EShipSystemIcon::Contracts};
        const FText Tips[]={INVTEXT("Career guilds and starter ships"),INVTEXT("Player organizations and membership"),INVTEXT("Applications and invitations")};
        for(int32 Index=0;Index<3;++Index)
            Tabs->AddSlot().AutoWidth().Padding(0,0,6,0)[Button(Icons[Index],Tips[Index],[this,Index](){Page=Index;Selected.Reset();SelectedMember=NAME_None;Refresh(0,0);})];
        auto Header=SNew(SHeaderRow)
            +SHeaderRow::Column(TEXT("Name")).DefaultLabel(INVTEXT("NAME")).FillWidth(2.4)
            +SHeaderRow::Column(TEXT("Kind")).DefaultLabel(INVTEXT("TYPE")).FillWidth(1)
            +SHeaderRow::Column(TEXT("Empire")).DefaultLabel(INVTEXT("FACTION")).FillWidth(1)
            +SHeaderRow::Column(TEXT("Status")).DefaultLabel(INVTEXT("STATUS")).FillWidth(1.5);
        SAssignNew(List,SListView<FRow>).ListItemsSource(&Rows).HeaderRow(Header).SelectionMode(ESelectionMode::Single)
            .OnGenerateRow_Lambda([](FRow Item,const TSharedRef<STableViewBase>& Owner){return SNew(SGuildRow,Owner).Item(Item);})
            .OnSelectionChanged_Lambda([this](FRow Row,ESelectInfo::Type){if(!bRefreshing){Selected=Row;SelectedMember=NAME_None;RefreshMembers();}});
        SAssignNew(Members,SListView<TSharedPtr<FGuildMember>>).ListItemsSource(&MemberRows).SelectionMode(ESelectionMode::Single)
            .OnGenerateRow_Lambda([this](TSharedPtr<FGuildMember> Member,const TSharedRef<STableViewBase>& Owner)
            {return SNew(STableRow<TSharedPtr<FGuildMember>>,Owner).Style(&MissionUIStyle::FlightTableRowStyle())
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Service.IsValid()?Service->GetPilotDisplayName(Member->Pilot).ToUpper():INVTEXT("UNKNOWN PILOT"))]
                +SHorizontalBox::Slot().FillWidth(2)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Roles(Member->Roles))).AutoWrapText(true)]];})
            .OnSelectionChanged_Lambda([this](TSharedPtr<FGuildMember> Member,ESelectInfo::Type){if(!bRefreshing)SelectedMember=Member?Member->Pilot:NAME_None;});
        auto CareerActions=SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[Button(EShipSystemIcon::Accept,INVTEXT("Enroll in the selected career guild"),[this](){Invoke([this](auto& API){return API.EnrollCareer(Definition(),Feedback);});},[this](){return !Definition().IsNone();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[Button(EShipSystemIcon::Hangar,INVTEXT("Claim the career starter ship while docked. Adds it to your hangar; mission-assigned ships are unchanged."),[this](){Invoke([this](auto& API){return API.ClaimStarter(Definition(),Feedback);});},[this](){return !Definition().IsNone();})];
        auto OrganizationActions=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(SEditableTextBox).HintText(INVTEXT("New organization name")).Font(MissionUIStyle::GetRowFont(12)).OnTextChanged_Lambda([this](FText Text){NewName=Text.ToString();})]
                +SHorizontalBox::Slot().AutoWidth()[Button(EShipSystemIcon::Guilds,INVTEXT("Create a player organization"),[this](){Invoke([this](auto& API){return API.CreateOrganization(NewName,Feedback);});})]
                +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[Button(EShipSystemIcon::Accept,INVTEXT("Apply to the selected organization"),[this](){Invoke([this](auto& API){return API.Apply(OrganizationId(),Feedback);});},[this](){return Organization()!=nullptr;})]
                +SHorizontalBox::Slot().AutoWidth()[Button(EShipSystemIcon::Exit,INVTEXT("Leave the selected organization. Leaders must transfer leadership first."),[this](){Invoke([this](auto& API){return API.Leave(OrganizationId(),Feedback);});},[this](){return Organization()!=nullptr;})]]
            +SVerticalBox::Slot().AutoHeight().Padding(0,5)[SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(SEditableTextBox).HintText(INVTEXT("Pilot name to invite")).Font(MissionUIStyle::GetRowFont(12)).OnTextChanged_Lambda([this](FText Text){InvitePilot=FName(*Text.ToString().TrimStartAndEnd());})]
                +SHorizontalBox::Slot().AutoWidth()[Button(EShipSystemIcon::Agent,INVTEXT("Invite this pilot. Recruitment permissions and faction rules apply."),[this](){Invoke([this](auto& API){return API.Invite(OrganizationId(),InvitePilot,Feedback);});},[this](){return Organization() && !InvitePilot.IsNone();})]];
        auto RoleActions=SNew(SHorizontalBox);
        const EGuildRole RoleValues[]={EGuildRole::Member,EGuildRole::Recruiter,EGuildRole::Quartermaster,EGuildRole::Treasurer,EGuildRole::Director};
        const EShipSystemIcon RoleIcons[]={EShipSystemIcon::Agent,EShipSystemIcon::Contracts,EShipSystemIcon::Cargo,EShipSystemIcon::Wallet,EShipSystemIcon::SettingsGear};
        for(int32 Index=0;Index<5;++Index)
            RoleActions->AddSlot().AutoWidth().Padding(0,0,5,0)[Button(RoleIcons[Index],FText::FromString(TEXT("Set selected member role: ")+StaticEnum<EGuildRole>()->GetNameStringByValue(static_cast<int64>(RoleValues[Index]))),
                [this,Role=RoleValues[Index]](){Invoke([this,Role](auto& API){return API.SetRoles(OrganizationId(),SelectedMember,static_cast<int32>(EGuildRole::Member)|static_cast<int32>(Role),Feedback);});},[this](){return Organization() && !SelectedMember.IsNone();})];
        RoleActions->AddSlot().AutoWidth()[Button(EShipSystemIcon::Override,INVTEXT("Transfer leadership to the selected member"),[this](){Invoke([this](auto& API){return API.TransferLeader(OrganizationId(),SelectedMember,Feedback);});},[this](){return Organization() && !SelectedMember.IsNone();})];
        auto RequestActions=SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()[Button(EShipSystemIcon::Accept,INVTEXT("Accept the selected invitation or approve the application"),[this](){Invoke([this](auto& API){return Selected && (Selected->bInvitation?API.AcceptInvitation(Selected->Id,Feedback):API.Review(Selected->Id,true,Feedback));});},[this](){return Selected.IsValid();})]
            +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[Button(EShipSystemIcon::Abort,INVTEXT("Reject the selected application"),[this](){Invoke([this](auto& API){return Selected && API.Review(Selected->Id,false,Feedback);});},[this](){return Selected && !Selected->bInvitation;})];
        RequestActions->AddSlot().AutoWidth().Padding(6,0)
            [Button(EShipSystemIcon::Back,INVTEXT("Decline your selected invitation or withdraw your application"),
                [this](){Invoke([this](auto& API){return Selected && API.CancelRequest(Selected->Id,Selected->bInvitation,Feedback);});},
                [this]()
                {
                    if(!Selected || !Service.IsValid())return false;
                    const auto& State=Service->GetState();const FName Pilot=Service->GetContext().Pilot;
                    if(Selected->bInvitation){const auto* Request=State.Invitations.Find(Selected->Id);return Request && Request->Pilot==Pilot;}
                    const auto* Request=State.Applications.Find(Selected->Id);return Request && Request->Pilot==Pilot;
                })];
        SAssignNew(Actions,SWidgetSwitcher)
            +SWidgetSwitcher::Slot()[CareerActions]
            +SWidgetSwitcher::Slot()[SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[OrganizationActions]
                +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(96)[MissionUIStyle::MakeDataPane(Members.ToSharedRef())]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,5)[RoleActions]]
            +SWidgetSwitcher::Slot()[RequestActions];
        ChildSlot[SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(8)[Tabs]
            +SVerticalBox::Slot().AutoHeight().Padding(8,0,8,6)[SNew(SSearchBox).HintText(INVTEXT("Search guilds and organizations")).OnTextChanged_Lambda([this](FText Text){Search=Text.ToString();Refresh(0,0);})]
            +SVerticalBox::Slot().FillHeight(.55f).Padding(8)[MissionUIStyle::MakeDataPane(List.ToSharedRef())]
            +SVerticalBox::Slot().FillHeight(.45f).Padding(8)[MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([this](){return Details();})]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,6)[Actions.ToSharedRef()])]
            +SVerticalBox::Slot().AutoHeight().Padding(8)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true).Text_Lambda([this](){return Feedback;})]];
        Refresh(0,0);RegisterActiveTimer(1.f,FWidgetActiveTimerDelegate::CreateSP(this,&SGuildPanel::Refresh));
    }
private:
    TSharedRef<SWidget> Button(EShipSystemIcon Icon,FText Tip,TFunction<void()> Action,TFunction<bool()> Enabled={})
    {return SNew(SButton).ToolTipText(Tip).IsEnabled_Lambda([this,Enabled](){return Service.IsValid() && (!Enabled || Enabled());})
        .OnClicked_Lambda([this,Action](){Action();Refresh(0,0);return FReply::Handled();})[SNew(SShipSystemIcon).Icon(Icon)];}
    void Invoke(TFunction<bool(UGuildCatalogSubsystem&)> Action)
    {if(Service.IsValid())Action(*Service.Get());else Feedback=INVTEXT("Guild services are unavailable.");}
    FName Definition()const{return Selected?Selected->Definition:NAME_None;}
    FGuid OrganizationId()const{return Selected?Selected->Id:FGuid();}
    const FGuildOrganization* Organization()const{return Service.IsValid()?Service->GetState().Organizations.Find(OrganizationId()):nullptr;}
    void RefreshMembers()
    {
        TGuardValue<bool> Guard(bRefreshing,true);
        const FName PreviousMember=SelectedMember;
        MemberRows.Reset();
        if(const auto* Org=Organization())for(const auto& Pair:Org->Members)MemberRows.Add(MakeShared<FGuildMember>(Pair.Value));
        MemberRows.Sort([](const auto& A,const auto& B){return A->Pilot.LexicalLess(B->Pilot);});
        if(Members)
        {
            Members->RequestListRefresh();Members->ClearSelection();
            for(const auto& Member:MemberRows)if(Member->Pilot==PreviousMember)
            {Members->SetSelection(Member,ESelectInfo::Direct);break;}
        }
    }
    EActiveTimerReturnType Refresh(double,float)
    {
        if(!Service.IsValid())return EActiveTimerReturnType::Continue;
        bRefreshing=true;const FRow Previous=Selected;Rows.Reset();
        const auto& State=Service->GetState();const auto Context=Service->GetContext();
        if(Page==0)
            for(const auto& Pair:Service->GetCatalog())if(!Context.Empire.IsNone() && Pair.Value.Empire==Context.Empire && Pair.Value.Kind==EGuildKind::Career)
            {
                auto Row=MakeShared<FGuildRow>();Row->Definition=Pair.Key;Row->Name=Pair.Value.DisplayName;Row->Kind=TEXT("CAREER");Row->Empire=Pair.Value.Empire.ToString();
                FText Reason;Row->Status=GuildAI::Eligible(Pair.Value,Context,Reason)?TEXT("AVAILABLE"):Reason.ToString();
                for(const auto& Org:State.Organizations)if(!Org.Value.bDisbanded && Org.Value.Definition==Pair.Key && Org.Value.Members.Contains(Context.Pilot))Row->Status=TEXT("ENROLLED");
                if(Search.IsEmpty() || (Row->Name+Row->Empire+Pair.Value.Description).Contains(Search,ESearchCase::IgnoreCase))Rows.Add(Row);
            }
        if(Page==1)for(const auto& Pair:State.Organizations)if(!Context.Empire.IsNone() && Pair.Value.Empire==Context.Empire && !Pair.Value.bDisbanded && Pair.Value.Kind==EGuildKind::PlayerOrganization)
        {
            auto Row=MakeShared<FGuildRow>();Row->Id=Pair.Key;Row->Definition=Pair.Value.Definition;Row->Name=Pair.Value.DisplayName;Row->Kind=TEXT("ORGANIZATION");Row->Empire=Pair.Value.Empire.ToString();
            Row->Status=Pair.Value.Members.Contains(Context.Pilot)?TEXT("MEMBER"):TEXT("RECRUITING");
            if(Search.IsEmpty() || (Row->Name+Row->Empire).Contains(Search,ESearchCase::IgnoreCase))Rows.Add(Row);
        }
        if(Page==2)
        {
            const int64 Now=FDateTime::UtcNow().ToUnixTimestamp();
            const auto AddRequest=[this,&State,&Context](FGuid Id,FGuid Group,FName Pilot,bool Invitation)
            {
                const auto* Org=State.Organizations.Find(Group);if(!Org || Org->bDisbanded || Context.Empire.IsNone() || Org->Empire!=Context.Empire)return;
                if(Pilot!=Context.Pilot && (Invitation || !(GuildAI::HasRole(*Org,Context.Pilot,EGuildRole::Leader) || GuildAI::HasRole(*Org,Context.Pilot,EGuildRole::Director) || GuildAI::HasRole(*Org,Context.Pilot,EGuildRole::Recruiter))))return;
                auto Row=MakeShared<FGuildRow>();Row->Id=Id;Row->bInvitation=Invitation;Row->Name=Org->DisplayName+TEXT(" / ")+Service->GetPilotDisplayName(Pilot).ToString();Row->Kind=Invitation?TEXT("INVITATION"):TEXT("APPLICATION");Row->Empire=Org->Empire.ToString();Row->Status=TEXT("PENDING");
                if(Search.IsEmpty() || Row->Name.Contains(Search,ESearchCase::IgnoreCase))Rows.Add(Row);
            };
            for(const auto& Pair:State.Applications)if(Pair.Value.State==EGuildRequestState::Pending && Pair.Value.ExpiresUtc>Now)AddRequest(Pair.Key,Pair.Value.Organization,Pair.Value.Pilot,false);
            for(const auto& Pair:State.Invitations)if(Pair.Value.State==EGuildRequestState::Pending && Pair.Value.ExpiresUtc>Now)AddRequest(Pair.Key,Pair.Value.Organization,Pair.Value.Pilot,true);
        }
        Rows.Sort([](const auto& A,const auto& B){return A->Name<B->Name;});Selected.Reset();
        List->RequestListRefresh();List->ClearSelection();
        if(Previous)for(const auto& Row:Rows)if(Row->Id==Previous->Id && Row->Definition==Previous->Definition && Row->bInvitation==Previous->bInvitation){Selected=Row;List->SetSelection(Row,ESelectInfo::Direct);break;}
        if(Actions)Actions->SetActiveWidgetIndex(Page);RefreshMembers();bRefreshing=false;return EActiveTimerReturnType::Continue;
    }
    FText Details()const
    {
        if(Service.IsValid() && Service->GetContext().Empire.IsNone())return INVTEXT("LOAD A PILOT WITH A KNOWN EMPIRE TO VIEW GUILDS.");
        if(!Selected)return INVTEXT("SELECT A CAREER, ORGANIZATION OR REQUEST. CAREER STARTER SHIPS ARE CLAIMED INTO YOUR HANGAR WHILE DOCKED; ASSIGNED MISSION SHIPS ARE UNCHANGED.");
        if(!Service.IsValid())return FText::GetEmpty();
        FString Text=Selected->Name+TEXT("\n")+Selected->Empire+TEXT(" | ")+Selected->Status;
        if(Page==0)
        {
            if(const auto* Def=Service->GetCatalog().Find(Definition()))
            {
                Text+=TEXT("\n")+Def->Description+FString::Printf(TEXT("\nMINIMUM RANK: %d | STARTER: %s"),Def->MinimumRank,*Def->StarterShip.ToString());
                for(const auto Skill:Def->RequiredSkills)Text+=TEXT("\nREQUIRES SKILL: ")+Skill.ToString();
                FText Reason;GuildAI::Eligible(*Def,Service->GetContext(),Reason);if(!Reason.IsEmpty())Text+=TEXT("\n")+Reason.ToString();
            }
        }
        else if(Page==1)
        {
            if(const auto* Org=Organization())Text+=FString::Printf(TEXT("\nLEADER: %s | MEMBERS: %d\nSELECT A MEMBER BELOW TO MANAGE ROLES. ROLE PERMISSIONS AND FACTION RESTRICTIONS ARE ENFORCED."),*Service->GetPilotDisplayName(Org->Leader).ToString(),Org->Members.Num());
        }
        else
        {
            if(Selected->bInvitation)
            {if(const auto* Request=Service->GetState().Invitations.Find(Selected->Id))Text+=TEXT("\nINVITED BY: ")+Service->GetPilotDisplayName(Request->InvitedBy).ToString()+TEXT("\nEXPIRES: ")+FDateTime::FromUnixTimestamp(Request->ExpiresUtc).ToString()+TEXT(" GMT");}
            else if(const auto* Request=Service->GetState().Applications.Find(Selected->Id))Text+=TEXT("\n")+Request->Message+TEXT("\nEXPIRES: ")+FDateTime::FromUnixTimestamp(Request->ExpiresUtc).ToString()+TEXT(" GMT");
        }
        return FText::FromString(Text.ToUpper());
    }
    TWeakObjectPtr<UGuildCatalogSubsystem> Service;
    TSharedPtr<SListView<FRow>> List;TArray<FRow> Rows;FRow Selected;
    TSharedPtr<SListView<TSharedPtr<FGuildMember>>> Members;TArray<TSharedPtr<FGuildMember>> MemberRows;
    TSharedPtr<SWidgetSwitcher> Actions;
    int32 Page=0;FName SelectedMember,InvitePilot;FString Search,NewName;FText Feedback;bool bRefreshing=false;
};
}
TSharedRef<SWidget> UGuildsDlg::RebuildWidget()
{return SNew(SGuildPanel).Instance(GetGameInstance());}
