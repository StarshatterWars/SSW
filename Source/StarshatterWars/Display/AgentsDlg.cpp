#include "AgentsDlg.h"
#include "PanelCloseIcon.h"
#include "StarshatterPlayerSubsystem.h"

#include "AgentAI.h"
#include "AgentMissionRules.h"
#include "AgentCatalogSubsystem.h"
#include "Widgets/Input/SComboBox.h"
#include "TimerManager.h"
#include "Widgets/SNullWidget.h"
#include "MenuScreen.h"
#include "Mission.h"
#include "Campaign.h"
#include "MissionsDlg.h"
#include "MissionObjectivesDlg.h"
#include "MissionUIStyle.h"
#include "PlayerCharacter.h"
#include "SSWGameInstance.h"
#include "ShipSystemIcon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    using State = AgentAI::EState;

    FName PilotId()
    {
        const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
        return Pilot ? AgentAI::PilotKey(*Pilot) : NAME_None;
    }
}

void UAgentsDlg::RefreshBoard()
{
    if (!Board) return;
    Board->ClearChildren();
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    if (!Catalog || PilotId().IsNone()) return;
    auto& Agent = Game->GetTrainingAgent();
    if (const auto* Officer = Catalog->FindAgent(SelectedAgent))
    {
        Board->AddSlot().AutoHeight().Padding(0,4,0,12)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
                .Text(Catalog->GetAssignmentStatus(SelectedAgent))];
        const auto* PlayerState=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        const bool bOperationAccepted=PlayerState && PlayerState->GetPlayerInfo().Campaign==Campaign::TRAINING_CAMPAIGN;
        if (Officer->Class == EAgentClass::Training)
        {
            Board->AddSlot().AutoHeight().Padding(0,0,0,12)
                [SNew(SButton).ContentPadding(FMargin(12,8))
                    .Text(bOperationAccepted?INVTEXT("OPERATION LIVE FIRE - ACCEPTED"):INVTEXT("ACCEPT OPERATION LIVE FIRE"))
                    .IsEnabled(!bOperationAccepted && PlayerState && PlayerState->GetPlayerInfo().Campaign==0 && CanLaunch.IsBound() && CanLaunch.Execute())
                    .ToolTipText(INVTEXT("Accept the training campaign. This assignment is saved to your pilot; individual lessons remain gated by progression."))
                    .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog]()
                    {
                        Feedback=Catalog->AssignTrainingOperation(SelectedAgent,false)?INVTEXT("Operation Live Fire accepted. Select T1 to review and accept your first lesson."):INVTEXT("The campaign assignment could not be saved.");
                        RefreshBoard();return FReply::Handled();
                    }))];
        }
        if (Officer->Class == EAgentClass::Training)
            for (int32 Id : Officer->MissionIds)
            {
                const auto* Offer = Agent.FindOffer(PilotId(), SelectedAgent);
                const bool bOffered = Offer && Offer->MissionId == Id;
                const bool bReplay = Catalog->CanReplayTrainingTest(Id,SelectedAgent);
                const auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
                const bool bCompleted=Agent.IsCompleted(PilotId(),Id) || (!Game->IsTrainingTestSession() && Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::TRAINING_CAMPAIGN,Id));
                const FLinearColor RowColor=bCompleted?FLinearColor(.35f,.9f,.45f):FLinearColor::White;
                const bool bSelectable = bOperationAccepted && (bReplay || (bOffered && Catalog->CanAccept(Id,SelectedAgent)));
                const FText Reason = !bOperationAccepted ? INVTEXT("Accept Operation Live Fire first") : bReplay ? INVTEXT("COMPLETE - REPLAY (NO POINTS)") : bCompleted ? INVTEXT("COMPLETE") : Catalog->GetTrainingRequirement(Id);
                Board->AddSlot().AutoHeight().Padding(0,3)
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(.44f).Padding(0,0,12,0)
                        [SNew(SButton).IsEnabled(bSelectable).ToolTipText(Reason)
                            .ButtonColorAndOpacity(bCompleted?FLinearColor(.08f,.28f,.12f):FLinearColor::White)
                            .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Id](){OpenDetails(Id,true);return FReply::Handled();}))
                            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).ColorAndOpacity(RowColor)
                                .Text(FText::Format(INVTEXT("T{0}  {1}"),FText::AsNumber(Id-100),Catalog->GetMissionTitle(Id)))]]
                        + SHorizontalBox::Slot().FillWidth(.56f).VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(Reason).ColorAndOpacity(RowColor).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(12))]];
            }
        const FText Orders = Catalog->GetCampaignOrders(SelectedAgent);
        if (!Orders.IsEmpty())
        {
            Board->AddSlot().AutoHeight().Padding(0,4,0,12)
                [SNew(STextBlock).Text(Orders).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(13))];
            Board->AddSlot().AutoHeight().Padding(0,4,0,12)
                [SNew(SButton).Text(INVTEXT("REQUEST CAMPAIGN ASSIGNMENT"))
                    .ToolTipText(INVTEXT("Request the next mission authorized by current campaign orders"))
                    .OnClicked(FOnClicked::CreateWeakLambda(this,[this](){RequestMission(true);return FReply::Handled();}))];
        }
        for (int32 CampaignId : Officer->CampaignIds)
        {
            if (CampaignId < 2 || Catalog->GetCampaignTitle(CampaignId).IsEmpty()) continue;
            Board->AddSlot().AutoHeight().Padding(0,3)
                [SNew(SButton).Text(FText::Format(INVTEXT("ACCEPT CAMPAIGN ORDERS: {0}"), Catalog->GetCampaignTitle(CampaignId)))
                    .ToolTipText(INVTEXT("Begin or resume these orders while docked. Previous campaign sections and flight training must be completed. Finish queued assignments first."))
                    .IsEnabled(CanLaunch.IsBound() && CanLaunch.Execute() && Catalog->CanActivateCampaign(SelectedAgent, CampaignId))
                    .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog,CampaignId]()
                    {
                        if (CanLaunch.IsBound() && CanLaunch.Execute() && Catalog->ActivateCampaign(SelectedAgent,CampaignId))
                        {
                            Feedback = INVTEXT("Campaign orders accepted. Request your next assignment from this agent.");
                            RefreshBoard();
                        }
                        return FReply::Handled();
                    }))];
        }
    }


    const auto AddMission = [this, Catalog](int32 Id, FText Status, bool bOffer, bool bHead)
    {
        const FText Title = Catalog->GetMissionTitle(Id);
        if (Title.IsEmpty()) return;
        Board->AddSlot().AutoHeight().Padding(0, 3)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [SNew(SButton).Text(Title)
                .OnClicked(FOnClicked::CreateWeakLambda(this, [this, Id, bOffer]()
                {
                    OpenDetails(Id, bOffer);
                    return FReply::Handled();
                }))]
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
            [SNew(SButton).ToolTipText(INVTEXT("Mission briefing and objectives"))
                .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Id,bOffer](){OpenDetails(Id,bOffer);return FReply::Handled();}))
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Computer)]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [SNew(STextBlock).Text(Status).Font(MissionUIStyle::GetRowFont(13))]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
            [SNew(SButton).ToolTipText_Lambda([this]()
                {
                    const auto* Game = Cast<USSWGameInstance>(GetGameInstance());
                    if (Game && Game->IsTrainingTestSession() && CanLaunch.IsBound() && CanLaunch.Execute()) return INVTEXT("Start the next Operation Live Fire lesson");
                    return CanLaunch.IsBound() && CanLaunch.Execute() ? INVTEXT("Open briefing for the next queued mission")
                        : INVTEXT("Dock before starting the next queued mission");
                })
                .Visibility(bHead ? EVisibility::Visible : EVisibility::Collapsed)
                .IsEnabled_Lambda([this]() { return CanLaunch.IsBound() && CanLaunch.Execute(); })
                .ContentPadding(FMargin(12,6))
                .OnClicked(FOnClicked::CreateWeakLambda(this, [this, Id]()
                {
                    StartMission(Id);
                    return FReply::Handled();
                }))[SNew(STextBlock).Text(INVTEXT("START MISSION")).Font(MissionUIStyle::GetRowFont(13))]]
        ];
    };

    if (const auto* Offer = Agent.FindOffer(PilotId(), SelectedAgent); Offer && Catalog->FindAgent(SelectedAgent) && Catalog->FindAgent(SelectedAgent)->Class != EAgentClass::Training)
        AddMission(Offer->MissionId, INVTEXT("OFFERED"), true, false);
    bool bHead = true;
    for (const auto& Mission : Agent.GetQueue(PilotId()))
    {
        AddMission(Mission.MissionId, Mission.State == State::InProgress ? INVTEXT("IN PROGRESS") : INVTEXT("QUEUED"),
            false, bHead && Mission.State == State::Accepted);
        bHead = false;
    }
}

void UAgentsDlg::StartMission(int32 MissionId)
{
    if (!CanLaunch.IsBound() || !CanLaunch.Execute()) return;
    auto* Menu = GetMenuManager();
    auto* World = GetWorld();
    if (!Menu || !World) return;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Menu, MissionId]()
    {
        if (!CanLaunch.IsBound() || !CanLaunch.Execute()) return;
        if (bStandaloneFrame) RemoveFromParent();
        Menu->OpenQueuedAssignment(MissionId, !bStandaloneFrame);
    }));
}

void UAgentsDlg::OpenDetails(int32 MissionId, bool bOffer)
{
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    if (!Catalog || !DetailLayer) return;
    const auto* Mission = Catalog->GetMission(MissionId);
    if (!Mission)
    {
        Feedback = INVTEXT("Mission details are unavailable.");
        return;
    }
    Details = CreateWidget<UMissionObjectivesDlg>(GetOwningPlayer());
    if (!Details) return;

    FSimpleDelegate Accept;
    if (bOffer)
        Accept = FSimpleDelegate::CreateWeakLambda(this, [this, MissionId, Issuer = SelectedAgent]()
        {
            auto* Instance = Cast<USSWGameInstance>(GetGameInstance());
            auto* Catalog = Instance ? Instance->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
            if (!Catalog) return;
            const bool bReplay=Catalog->CanReplayTrainingTest(MissionId,Issuer);
            const bool bAccepted=bReplay?Instance->GetTrainingAgent().QueueTestReplay(PilotId(),MissionId,Issuer)
                :(Catalog->CanAccept(MissionId,Issuer) && Instance->GetTrainingAgent().Accept(PilotId(),MissionId));
            if(!bAccepted)
            {
                Feedback=INVTEXT("This offer expired or is no longer available. Request a new assignment.");
                CloseDetails();RefreshBoard();return;
            }
            if(MissionId>=101 && MissionId<=107 && !Catalog->AssignTrainingOperation(Issuer))
            {
                Feedback=INVTEXT("Mission accepted, but the operation assignment could not be saved. Please retry before launch.");
            }
            if (auto* Players = Instance->GetSubsystem<UStarshatterPlayerSubsystem>()) Players->SavePlayer(true);
            CloseDetails();
            RefreshBoard();
            OpenDetails(MissionId, false);
        });
    FSimpleDelegate Decline;
    const auto Queue = Game->GetTrainingAgent().GetQueue(PilotId());
    if (bOffer || Queue.ContainsByPredicate([MissionId](const AgentAI::FAssignment& Item)
        { return Item.MissionId == MissionId && Item.State == State::Accepted; }))
        Decline = FSimpleDelegate::CreateWeakLambda(this, [this, MissionId]()
        {
            if (auto* Instance = Cast<USSWGameInstance>(GetGameInstance()))
                Instance->GetTrainingAgent().Decline(PilotId(), MissionId);
            CloseDetails();
            RefreshBoard();
        });

    FName Issuer=SelectedAgent; FText Status=Catalog->CanReplayTrainingTest(MissionId,Issuer)?INVTEXT("TEST REPLAY - NO POINTS OR PROGRESSION"):bOffer ? INVTEXT("OFFERED") : INVTEXT("QUEUED");
    for (const auto& Assignment : Game->GetTrainingAgent().GetQueue(PilotId()))
        if (Assignment.MissionId==MissionId) { Issuer=Assignment.Issuer; if(Assignment.State==State::InProgress)Status=INVTEXT("IN PROGRESS"); break; }
    if(bOffer)
        if(const auto* Offer=Game->GetTrainingAgent().FindOffer(PilotId(),Issuer); Offer && Offer->ExpiresUtc>0)
            Status=FText::Format(INVTEXT("{0} | OFFER EXPIRES {1} GMT"),Status,FText::FromString(FDateTime::FromUnixTimestamp(Offer->ExpiresUtc).ToString(TEXT("%H:%M:%S"))));
    const auto* Officer=Catalog->FindAgent(Issuer);
    CastChecked<UMissionObjectivesDlg>(Details)->SetMissionBriefing(*Mission,Catalog->GetMissionTitle(MissionId),
        Officer ? FText::FromString(Officer->DisplayName) : INVTEXT("Mission Control"),Status,
        MoveTemp(Accept),MoveTemp(Decline),FSimpleDelegate::CreateUObject(this,&UAgentsDlg::CloseDetails));
    if (!Queue.IsEmpty() && Queue[0].MissionId == MissionId && Queue[0].State == State::Accepted)
        Details->SetStartMission(FSimpleDelegate::CreateWeakLambda(this,[this,MissionId](){StartMission(MissionId);}),CanLaunch);
    DetailLayer->SetContent(Details->TakeWidget());
    DetailLayer->SetVisibility(EVisibility::Visible);
}

void UAgentsDlg::CloseDetails()
{
    if (DetailLayer)
    {
        DetailLayer->SetContent(SNullWidget::NullWidget);
        DetailLayer->SetVisibility(EVisibility::Collapsed);
    }
    Details = nullptr;
}

void UAgentsDlg::RequestMission(bool bCampaign)
{
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    if (!Catalog || !Catalog->CanContact(SelectedAgent)) return;
    auto Eligible = Catalog->GetEligibleMissions(SelectedAgent);
    Eligible.RemoveAll([bCampaign](int32 Id){return (Id >= AgentMissionRules::CampaignBase) != bCampaign;});
    const auto* Offer = Game->GetTrainingAgent().Request(PilotId(), Eligible, SelectedAgent, true);
    Feedback = Offer ? FText::GetEmpty() : INVTEXT("No assignments are available for this agent and rank.");
    RefreshBoard();
    if (auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>()) Players->SavePlayer(true);
    if (Offer) OpenDetails(Offer->MissionId, true);
}

TSharedRef<SWidget> UAgentsDlg::RebuildWidget()
{
    SetIsFocusable(true);
    auto* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    const auto* Table = Catalog ? Catalog->GetCatalog() : nullptr;
    AgentChoices.Reset();
    TSharedPtr<FName> InitialAgent;
    if (Table)
    {
        auto Names = Table->GetRowNames();
        Names.Sort([Catalog](FName A, FName B)
        {
            const auto& Left = *Catalog->FindAgent(A);
            const auto& Right = *Catalog->FindAgent(B);
            const int32 SystemOrder = Left.System.Compare(Right.System);
            if (SystemOrder != 0) return SystemOrder < 0;
            if (Left.Level != Right.Level) return Left.Level < Right.Level;
            return Left.DisplayName < Right.DisplayName;
        });
        for (FName Id : Names)
        {
            if (!Catalog->CanContact(Id)) continue;
            auto Item = MakeShared<FName>(Id);
            AgentChoices.Add(Item);
            if (Id == SelectedAgent || (!InitialAgent && Catalog->FindAgent(Id)->Class == EAgentClass::Training))
                InitialAgent = Item;
        }
    }
    if (!InitialAgent && !AgentChoices.IsEmpty()) InitialAgent = AgentChoices[0];
    SelectedAgent = InitialAgent ? *InitialAgent : NAME_None;

    const auto AgentLabel = [Catalog](FName Id)
    {
        const auto* Agent = Catalog ? Catalog->FindAgent(Id) : nullptr;
        return Agent ? FText::FromString(FString::Printf(TEXT("%s   /   %s"), *Agent->DisplayName, *Agent->System))
            : INVTEXT("No agents for your faction");
    };
    const auto Content = SNew(SOverlay)
        + SOverlay::Slot()
        [
            MissionUIStyle::MakeDataPane(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                [
                    SNew(SComboBox<TSharedPtr<FName>>).OptionsSource(&AgentChoices).InitiallySelectedItem(InitialAgent)
                    .OnGenerateWidget_Lambda([AgentLabel](TSharedPtr<FName> Item)
                    {
                        return SNew(STextBlock).Text(AgentLabel(*Item)).Font(MissionUIStyle::GetRowFont(14));
                    })
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FName> Item, ESelectInfo::Type)
                    {
                        SelectedAgent = Item ? *Item : NAME_None;
                        Feedback = FText::GetEmpty();
                        CloseDetails();
                        if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()); Game && SelectedAgent == TEXT("JanekFlightTraining"))
                            if (auto* Catalog = Game->GetSubsystem<UAgentCatalogSubsystem>())
                                Game->GetTrainingAgent().Request(PilotId(), Catalog->GetEligibleMissions(SelectedAgent), SelectedAgent, true);
                        RefreshBoard();
                    })
                    [SNew(STextBlock).Text_Lambda([this, AgentLabel]() { return AgentLabel(SelectedAgent); })
                        .Font(MissionUIStyle::GetHeaderFont(15))]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 12, 12)
                [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
                    .Text_Lambda([this, Catalog]()
                    {
                        const auto* Agent = Catalog ? Catalog->FindAgent(SelectedAgent) : nullptr;
                        return Agent ? FText::FromString(FString::Printf(TEXT("%s  /  LEVEL %d\n%s  /  %s\n%s"),
                            *Agent->Division, Agent->Level, *Agent->Planet, *Agent->Station, *Agent->Description)) : FText::GetEmpty();
                    })]
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Text(INVTEXT("MISSION QUEUE")).Font(MissionUIStyle::GetHeaderFont(15))]
                + SVerticalBox::Slot().FillHeight(1)
                [SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Board, SVerticalBox)]]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                [SNew(STextBlock).Text_Lambda([this]() { return Feedback; }).AutoWrapText(true)]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
                [SNew(SButton).ToolTipText(INVTEXT("Request an assignment"))
                    .IsEnabled_Lambda([this, Catalog]() { return Catalog && Catalog->CanContact(SelectedAgent); })
                    .OnClicked(FOnClicked::CreateWeakLambda(this, [this]()
                    {
                        RequestMission();
                        return FReply::Handled();
                    }))[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Agent)]])
        ]
        + SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
        [SAssignNew(DetailLayer, SBox).Visibility(EVisibility::Collapsed)];
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()); Game && Catalog && SelectedAgent == TEXT("JanekFlightTraining"))
    {
        const auto Eligible = Catalog->GetEligibleMissions(SelectedAgent);
        Game->GetTrainingAgent().Request(PilotId(), Eligible, SelectedAgent, true);
    }
    RefreshBoard();
    if (!bStandaloneFrame) return Content;
    return SNew(SOverlay)
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(760).HeightOverride(410)
            [MissionUIStyle::MakeDataPane(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1)
                    [SNew(STextBlock).Text(INVTEXT("AGENTS")).Font(MissionUIStyle::GetHeaderFont(17))]
                    + SHorizontalBox::Slot().AutoWidth()
                    [SNew(SButton).ContentPadding(FMargin(8)).HAlign(HAlign_Center).VAlign(VAlign_Center).ToolTipText(INVTEXT("Close panel")).ButtonColorAndOpacity(FLinearColor(.55f, .09f, .07f))
                        .OnClicked(FOnClicked::CreateWeakLambda(this, [this]()
                        {
                            CloseDetails();
                            RemoveFromParent();
                            return FReply::Handled();
                        }))[SNew(SPanelCloseIcon)]]
                ]
                + SVerticalBox::Slot().FillHeight(1)[Content])]
        ];
}

FReply UAgentsDlg::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape)
    {
        if (Details) { CloseDetails(); return FReply::Handled(); }
        if (bStandaloneFrame) { RemoveFromParent(); return FReply::Handled(); }
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UAgentsDlg::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Board.Reset();
    DetailLayer.Reset();
}
