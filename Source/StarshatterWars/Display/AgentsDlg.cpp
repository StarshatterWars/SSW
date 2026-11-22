
#include "AgentsDlg.h"
#include "StationServices.h"
#include "FreeFlightAssignment.h"
#include "FactionTrainingCatalog.h"
#include "RecordDetails.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "Sim.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "PanelCloseIcon.h"
#include "StarshatterPlayerSubsystem.h"

#include "AgentAI.h"
#include "DynamicMissionAI.h"
#include "MissionArcRules.h"
#include "FactionBriefingCatalog.h"
#include "MissionRewardRules.h"
#include "AgentJournalTransaction.h"
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
#include "FlightPanelStyle.h"
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

    TFunction<bool(int32)> RegionalHistory(USSWGameInstance* Game)
    {
        const TWeakObjectPtr<UStarshatterPlayerSubsystem> Players = Game ? Game->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
        return [Players](int32 Id)
        {
            return Players.IsValid() && Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, Id);
        };
    }

    bool CommitAgentCommand(USSWGameInstance* Game, TFunctionRef<bool()> Mutate, FText& Feedback)
    {
        auto* Players=Game ? Game->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
        if (!Players) { Feedback=INVTEXT("Pilot save is unavailable. No assignment was changed."); return false; }
        const auto Result=AgentJournalTransaction::Commit(Game->GetTrainingAgent(),Players->GetMutablePlayerInfo(),Mutate,
            [Game,Players]() { return Game->IsTrainingTestSession() || Players->SaveActivePlayerProgress(); });
        if (Result==AgentJournalTransaction::EResult::SaveFailed)
            Feedback=INVTEXT("Unable to save the assignment change. Your previous assignment was restored. Please retry.");
        return Result==AgentJournalTransaction::EResult::Committed;
    }

    FName PilotId()
    {
        const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
        return Pilot ? AgentAI::PilotKey(*Pilot) : NAME_None;
    }
}

void UAgentsDlg::RefreshBoard()
{
    bJournalDirty=false;
    if (!Board) return;
    Board->ClearChildren();
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    if (!Catalog || PilotId().IsNone()) return;
    auto& Agent = Game->GetTrainingAgent();
    const int32 TransferStage=Catalog->GetTrainingTransferStage();
    if(TransferStage)
        Board->AddSlot().AutoHeight().Padding(0,4,0,12)
            [SNew(SButton).Text(TransferStage==1?INVTEXT("TDY TO JANEK STATION"):INVTEXT("RETURN TO ORION"))
                .ToolTipText(TransferStage==1?INVTEXT("Begin temporary duty at Janek Station for T3 and T4. This does not accept a new campaign."):
                    INVTEXT("End your TDY and return aboard Orion. Campaign 2 orders and fleet deployment are separate steps."))
                .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog]()
                {Catalog->TransferTrainingPilot(Feedback);RefreshBoard();return FReply::Handled();}))];
    if(Catalog->NeedsCampaignDeployment(2) && Catalog->IsAboardTrainingCarrier())
        Board->AddSlot().AutoHeight().Padding(0,4,0,12)
            [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
                    .Text(INVTEXT("CAMPAIGN 2 ORDERS ACCEPTED. Deploy Orion and its group to Meridian when ready."))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,6)
                    [SNew(SButton).Text(INVTEXT("DEPLOY ORION GROUP TO MERIDIAN"))
                        .ToolTipText(INVTEXT("Begin fleet deployment while aboard Orion. This does not perform a training shuttle transfer."))
                        .IsEnabled(FStationServices::IsDocked())
                        .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog]()
                        {Catalog->DeployCampaignFleet(2,Feedback);RefreshBoard();return FReply::Handled();}))])];
    const FName NextOfficer=Catalog->GetPostTrainingCampaignAgent();
    if(const auto* Referral=Catalog->FindAgent(NextOfficer);Referral && NextOfficer!=SelectedAgent)
    {
        Board->AddSlot().AutoHeight().Padding(0,4,0,10)
            [MissionUIStyle::MakeDataPane(SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(14))
                    .Text(INVTEXT("FLIGHT QUALIFICATIONS COMPLETE"))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,6)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)
                    .Text(FText::FromString(FString::Printf(TEXT("Your first four lessons are complete. %s has your Campaign 2 orders at %s, %s, %s System. Review and accept the next operation when docked."),
                        *Referral->DisplayName,*Referral->Station,*Referral->Region,*Referral->System)))]
                +SVerticalBox::Slot().AutoHeight()[SNew(SButton).Text(INVTEXT("VIEW CAMPAIGN 2 ORDERS"))
                    .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog,NextOfficer]()
                    {
                        if(const auto* Officer=Catalog->FindAgent(NextOfficer))
                        {
                            CloseDetails();DirectoryScope=EAgentDirectoryScope::EmpireWide;
                            SelectedSystem=FName(*Officer->System);SelectedAgent=NextOfficer;
                            RefreshDirectory();RefreshBoard();
                        }
                        return FReply::Handled();
                    }))])];
    }
    Board->AddSlot().AutoHeight().Padding(8,4,8,8)[SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(INVTEXT("ASSIGNMENT / STATUS")).Font(MissionUIStyle::GetRowFont(10))]
        + SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(INVTEXT("OFFER EXPIRY (GMT)")).Font(MissionUIStyle::GetRowFont(10))]];
    if (const auto* Officer = Catalog->FindAgent(SelectedAgent))
    {
        if(Officer->Class==EAgentClass::Training && !Officer->Description.IsEmpty())
            Board->AddSlot().AutoHeight().Padding(0,4,0,10)
                [SNew(STextBlock).Text(FText::FromString(Officer->Description)).Font(MissionUIStyle::GetRowFont(12)).AutoWrapText(true)];
        Board->AddSlot().AutoHeight().Padding(0,4,0,12)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).AutoWrapText(true)
                .Text(Catalog->GetAssignmentStatus(SelectedAgent))];
        if(!Officer->IntroductionId.IsNone())
            Board->AddSlot().AutoHeight().Padding(0,0,0,10)
            [SNew(SButton).ContentPadding(FMargin(12,8))
                .ToolTipText(INVTEXT("Read your faction's training or campaign introduction"))
                .OnClicked_Lambda([this,Id=Officer->IntroductionId](){OpenIntroduction(Id);return FReply::Handled();})
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth()[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Intel)]
                    +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(STextBlock).Text(INVTEXT("CAMPAIGN BRIEFING")).Font(MissionUIStyle::GetRowFont(12))]]];
        const auto* PlayerState=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        const bool bOperationAccepted=PlayerState && PlayerState->GetPlayerInfo().Campaign==Campaign::TRAINING_CAMPAIGN;
        if (Officer->Class == EAgentClass::Training)
        {
            Board->AddSlot().AutoHeight().Padding(0,0,0,12)
                [SNew(SButton).ContentPadding(FMargin(12,8))
                    .Text(FText::Format(bOperationAccepted?INVTEXT("{0} - ACCEPTED"):INVTEXT("ACCEPT {0}"),FText::FromString(FactionTrainingCatalog::Title(Officer->Empire)).ToUpper()))
                    .IsEnabled(!bOperationAccepted && PlayerState && PlayerState->GetPlayerInfo().Campaign==0 && CanLaunch.IsBound() && CanLaunch.Execute())
                    .ToolTipText(INVTEXT("Accept the training campaign. This assignment is saved to your pilot; individual lessons remain gated by progression."))
                    .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Catalog]()
                    {
                        Feedback=Catalog->AssignTrainingOperation(SelectedAgent,false)?INVTEXT("Training operation accepted. Select T1 to review and accept your first lesson."):INVTEXT("The campaign assignment could not be saved.");
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
                const FText Reason = !bOperationAccepted ? INVTEXT("Accept your training operation first") : bReplay ? INVTEXT("COMPLETE - REPLAY (NO REWARDS)") : bCompleted ? INVTEXT("COMPLETE") : Catalog->GetTrainingRequirement(Id);
                Board->AddSlot().AutoHeight().Padding(0,3)
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(.44f).Padding(0,0,12,0)
                        [SNew(SButton).IsEnabled(bSelectable).ToolTipText(Reason)
                            .ButtonColorAndOpacity(bCompleted?FLinearColor(.08f,.28f,.12f):FLinearColor::White)
                            .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Id](){OpenDetails(Id,true);return FReply::Handled();}))
                            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).ColorAndOpacity(RowColor)
                                .AutoWrapText(true).Text(FText::Format(INVTEXT("T{0}  {1}"),FText::AsNumber(Id-100),Catalog->GetMissionTitle(Id)))]]
                        + SHorizontalBox::Slot().FillWidth(.56f).VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(Reason).ColorAndOpacity(RowColor).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(11))]
                        + SHorizontalBox::Slot().AutoWidth().Padding(12,0,4,0).VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(INVTEXT("NO EXPIRY")).Font(MissionUIStyle::GetRowFont(10)).ColorAndOpacity(FLinearColor(.65f,.75f,.8f))]];
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
                            Feedback = CampaignId==2?INVTEXT("Campaign 2 orders accepted. Deploy Orion's group to Meridian when ready."):
                                INVTEXT("Campaign orders accepted. Request your next assignment from this agent.");
                            RefreshBoard();
                        }
                        else
                        {
                            Feedback = INVTEXT("Campaign orders could not be accepted. Check docking, rank, previous operations and save availability.");
                            RefreshBoard();
                        }
                        return FReply::Handled();
                    }))];
        }
    }


    const auto AddMission = [this, Catalog, Game](int32 Id, FText Status, bool bOffer, bool bHead)
    {
        FText Expiry=INVTEXT("NO DEADLINE");
        if(bOffer)if(const auto* Offer=Game->GetTrainingAgent().FindOffer(PilotId(),SelectedAgent);Offer && Offer->ExpiresUtc>0)
            Expiry=FText::FromString(FDateTime::FromUnixTimestamp(Offer->ExpiresUtc).ToString(TEXT("%d %b %H:%M")));
        const FText Title = Catalog->GetMissionTitle(Id);
        if (Title.IsEmpty()) return;
        Board->AddSlot().AutoHeight().Padding(0, 3)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [SNew(SButton)
                .OnClicked(FOnClicked::CreateWeakLambda(this, [this, Id, bOffer]()
                {
                    OpenDetails(Id, bOffer);
                    return FReply::Handled();
                }))[SNew(STextBlock).Text(Title).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(12))]]
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
            [SNew(SButton).ToolTipText(INVTEXT("Mission briefing and objectives"))
                .OnClicked(FOnClicked::CreateWeakLambda(this,[this,Id,bOffer](){OpenDetails(Id,bOffer);return FReply::Handled();}))
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Computer)]]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [SNew(STextBlock).Text(Status).Font(MissionUIStyle::GetRowFont(11)).ColorAndOpacity(FLinearColor(.4f,.8f,.9f))]
            + SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
            [SNew(SButton).ToolTipText_Lambda([this]()
                {
                    const auto* Game = Cast<USSWGameInstance>(GetGameInstance());
                    if (Game && Game->IsTrainingTestSession() && CanLaunch.IsBound() && CanLaunch.Execute()) return INVTEXT("Start the next Operation Live Fire lesson");
                    return CanLaunch.IsBound() && CanLaunch.Execute() ? INVTEXT("Open briefing for the next queued mission")
                        : INVTEXT("Dock before starting the next queued mission");
                })
                .Visibility(bHead ? EVisibility::Visible : EVisibility::Collapsed)
                .IsEnabled_Lambda([this, Id]() { return CanStartMission(Id); })
                .ContentPadding(FMargin(12,6))
                .OnClicked(FOnClicked::CreateWeakLambda(this, [this, Id]()
                {
                    StartMission(Id);
                    return FReply::Handled();
                }))[SNew(SHorizontalBox) + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Play)] + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6,0)[SNew(STextBlock).Text(INVTEXT("MISSION")).Font(MissionUIStyle::GetRowFont(13))]]]
            + SHorizontalBox::Slot().AutoWidth().Padding(12,0,4,0).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(Expiry).Justification(ETextJustify::Right).Font(MissionUIStyle::GetRowFont(10)).ColorAndOpacity(FLinearColor(.65f,.75f,.8f))]
        ];
    };

    if (const auto* Offer = Agent.FindOffer(PilotId(), SelectedAgent); Offer && Catalog->FindAgent(SelectedAgent) && Catalog->FindAgent(SelectedAgent)->Class != EAgentClass::Training)
        AddMission(Offer->MissionId, INVTEXT("OFFERED"), true, false);
    if(const auto* Outcome=Agent.LatestOutcome(PilotId());Outcome && Outcome->State==State::Completed)
    {
        const auto Progress=FMissionArcCatalog::Get().Evaluate(Outcome->MissionId,[&](int32 Id){return Agent.IsCompleted(PilotId(),Id) || RegionalHistory(Game)(Id);},[&](int32 Id){return Agent.HasChosen(PilotId(),Id);});
        if(Progress.Arc)for(int32 Index:Progress.Available)
        {
            const auto& Next=Progress.Arc->Steps[Index];const FName Target=Next.AgentId;
            Board->AddSlot().AutoHeight().Padding(0,8)[SNew(SButton)
                .IsEnabled(Catalog->FindAgent(Target)!=nullptr)
                .ToolTipText(FText::FromString(Next.Title))
                .OnClicked_Lambda([this,Target](){DirectoryScope=EAgentDirectoryScope::EmpireWide;SelectedAgent=Target;if(const auto* C=GetGameInstance()->GetSubsystem<UAgentCatalogSubsystem>())if(const auto* A=C->FindAgent(Target))SelectedSystem=FName(*A->System);RefreshDirectory();return FReply::Handled();})
                [SNew(STextBlock).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(12))
                    .Text(FText::FromString(FString::Printf(TEXT("NEXT AGENT: %s  /  %s - %s"),*Next.Issuer,*Next.System,*Next.Region)))]];
        }
    }
    const auto Journal=Agent.GetHistory(PilotId());int32 Shown=0;
    for(int32 Index=Journal.Num()-1;Index>=0 && Shown<3;--Index)
    {
        const auto& Item=Journal[Index];if(Item.Issuer!=SelectedAgent)continue;++Shown;
        const TCHAR* Label=Item.State==State::Completed?TEXT("COMPLETED"):Item.State==State::Failed?TEXT("FAILED"):TEXT("ABANDONED");
        const int64 Date=Item.AbandonedUtc>0?Item.AbandonedUtc:Item.CompletedUtc;
        const FString When=Date>0?FDateTime::FromUnixTimestamp(Date).ToString(TEXT("%d %b %H:%M GMT")):TEXT("DATE NOT RECORDED");
        Board->AddSlot().AutoHeight().Padding(8,4)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::Format(INVTEXT("{0}  /  {1}"),Catalog->GetMissionTitle(Item.MissionId),FText::FromString(Label))).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(10))]
            + SHorizontalBox::Slot().AutoWidth().Padding(12,0)[SNew(STextBlock).Text(FText::FromString(When)).Font(MissionUIStyle::GetRowFont(10))]];
    }
    bool bHead = true;
    for (const auto& Mission : Agent.GetQueue(PilotId()))
    {
        AddMission(Mission.MissionId, Mission.State == State::InProgress ? INVTEXT("IN PROGRESS") : INVTEXT("QUEUED"),
            false, bHead && Mission.State == State::Accepted);
        bHead = false;
    }
}

bool UAgentsDlg::CanStartMission(int32 MissionId) const
{
    if(MissionId==FreeFlightAssignment::Id)return FStationServices::IsDocked();
    if((MissionId==103 || MissionId==104) && PlayerCharacter::GetCurrentPlayer() && PlayerCharacter::GetCurrentPlayer()->GetEmpire()==EEMPIRE_NAME::Terellian)
        if(auto* S=Sim::GetSim()) if(S->GetMission() && S->GetMission()->IsFreeFlight()) return true;
    if (!DynamicMissionAI::IsDynamic(MissionId)) return CanLaunch.IsBound() && CanLaunch.Execute();
    auto* Simulation = Sim::GetSim();
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Players = Game ? Game->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
    FText Reason;
    return Simulation && Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight()
        && Players && DynamicMissionAI::CanUse(Players->GetPlayerInfo(), MissionId, Reason);
}

void UAgentsDlg::StartMission(int32 MissionId)
{
    if (!CanStartMission(MissionId)) return;
    auto* Menu = GetMenuManager();
    auto* World = GetWorld();
    if (!Menu || !World) return;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Menu, MissionId]()
    {
        if (!CanStartMission(MissionId)) return;
        // Surface lessons can request transit without changing the live mission.
        // Keep the briefing open and show its instructions instead of hiding the result behind it.
        FText Reason;
        const bool Started = Menu->OpenQueuedAssignment(MissionId, !bStandaloneFrame, &Reason);
        if (!Started && Reason.IsEmpty())
            Reason = INVTEXT("The mission could not start. Reopen Agents and check your assignment.");
        Feedback = Reason;
        if (IsValid(Details)) Details->SetLaunchFeedback(Reason);
        UE_LOG(LogTemp, Display, TEXT("[Agents] Mission %d start %s: %s"), MissionId,
            Started ? TEXT("accepted") : TEXT("blocked"), *Reason.ToString());
        if (Started && !bStandaloneFrame && (MissionId == 103 || MissionId == 104) && !Reason.IsEmpty()
            && OnTransitStarted.IsBound())
        {
            // Return to the live launch-bay camera so the pilot can see Orion jump.
            OnTransitStarted.Execute();
            return;
        }
        if (Started && Reason.IsEmpty() && bStandaloneFrame) RemoveFromParent();
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
            const auto* Players=Instance->GetSubsystem<UStarshatterPlayerSubsystem>();
            const bool bLesson=MissionId>=101 && MissionId<=107;
            if (bLesson && !Instance->IsTrainingTestSession() && (!Players || Players->GetPlayerInfo().Campaign==0))
            {
                Feedback=INVTEXT("Accept Operation Live Fire before accepting a training lesson.");
                CloseDetails(); RefreshBoard(); return;
            }
            const bool bReplay=Catalog->CanReplayTrainingTest(MissionId,Issuer);
            Feedback=INVTEXT("This offer expired or is no longer available. Request a new assignment.");
            const bool bAccepted=CommitAgentCommand(Instance,[&]()
            {
                return bReplay ? Instance->GetTrainingAgent().QueueTestReplay(PilotId(),MissionId,Issuer,
                    [Players](int32 Id){return Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::TRAINING_CAMPAIGN,Id);},
                    FStationServices::IsDocked())
                    : Catalog->CanAccept(MissionId,Issuer) && Instance->GetTrainingAgent().Accept(PilotId(),MissionId,RegionalHistory(Instance));
            },Feedback);
            if (!bAccepted) { CloseDetails(); RefreshBoard(); return; }
            Feedback=FText::GetEmpty();
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
            {
                Feedback=INVTEXT("This assignment can no longer be declined.");
                if (CommitAgentCommand(Instance,[&]() { return Instance->GetTrainingAgent().Decline(PilotId(), MissionId); },Feedback))
                    Feedback=INVTEXT("Assignment declined.");
            }
            CloseDetails();
            RefreshBoard();
        });

    FName Issuer=SelectedAgent; FText Status=Catalog->CanReplayTrainingTest(MissionId,Issuer)?INVTEXT("TRAINING REPLAY - NO REWARDS OR PROGRESSION"):bOffer ? INVTEXT("OFFERED") : INVTEXT("QUEUED");
    for (const auto& Assignment : Game->GetTrainingAgent().GetQueue(PilotId()))
        if (Assignment.MissionId==MissionId) { Issuer=Assignment.Issuer; if(Assignment.State==State::InProgress)Status=INVTEXT("IN PROGRESS"); break; }
    FText Expiry=AgentAI::IsTrainingMission(MissionId)?INVTEXT("NO EXPIRY"):INVTEXT("NO DEADLINE");
    if(bOffer && !AgentAI::IsTrainingMission(MissionId))
        if(const auto* Offer=Game->GetTrainingAgent().FindOffer(PilotId(),Issuer);Offer && Offer->ExpiresUtc>0)
            Expiry=FText::Format(INVTEXT("EXPIRES {0} GMT"),FText::FromString(FDateTime::FromUnixTimestamp(Offer->ExpiresUtc).ToString(TEXT("%d %b %H:%M"))));
    const auto* Officer=Catalog->FindAgent(Issuer);
    CastChecked<UMissionObjectivesDlg>(Details)->SetMissionBriefing(*Mission,Catalog->GetMissionTitle(MissionId),
        Officer ? FText::FromString(Officer->DisplayName) : INVTEXT("Mission Control"),Status,
        MoveTemp(Accept),MoveTemp(Decline),FSimpleDelegate::CreateUObject(this,&UAgentsDlg::CloseDetails));
    int32 RewardCampaign=AgentAI::IsTrainingMission(MissionId)?1:1000,RewardMission=MissionId;
    AgentMissionRules::Decode(MissionId,RewardCampaign,RewardMission);
    auto Reward=FMissionRewardCatalog::Get().Quote(RewardCampaign,RewardMission,false,0,0);
    if(DynamicMissionAI::IsDynamic(MissionId))if(auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>()){Reward.Base=DynamicMissionAI::Reward(Players->GetPlayerInfo(),MissionId);Reward.CompletionPoints=DynamicMissionAI::Points(Players->GetPlayerInfo(),MissionId);Reward.Recovery=Reward.Objectives=Reward.Time=0;}
    if(MissionId==FreeFlightAssignment::Id){Reward.Base=Reward.CompletionPoints=Reward.Recovery=Reward.Objectives=Reward.Time=0;}
    if(MissionId==FreeFlightAssignment::Id)
        Status=FText::Format(INVTEXT("{0} / INDEPENDENT FLIGHT / ACTIVE CRAFT / NO REWARDS"),Status);
    else
        Status=FText::Format(INVTEXT("{0}  /  {1} POINTS  /  {2} CR + ELIGIBLE BONUSES"),Status,FText::AsNumber(Reward.CompletionPoints),FText::AsNumber(Reward.Base));
    for(const auto& Assignment:Queue)if(Assignment.MissionId==MissionId && Assignment.AcceptedUtc>0)
        Status=FText::Format(INVTEXT("{0}\nACCEPTED {1} GMT"),Status,FText::FromString(FDateTime::FromUnixTimestamp(Assignment.AcceptedUtc).ToString(TEXT("%d %b %H:%M"))));
    Details->SetAssignmentMetadata(Officer?FText::FromString(Officer->DisplayName):INVTEXT("Mission Control"),Status,Expiry);
    if (!Queue.IsEmpty() && Queue[0].MissionId == MissionId && Queue[0].State == State::Accepted)
        Details->SetStartMission(FSimpleDelegate::CreateWeakLambda(this,[this,MissionId](){StartMission(MissionId);}),TDelegate<bool()>::CreateWeakLambda(this,[this,MissionId](){return CanStartMission(MissionId);}));
    DetailLayer->SetContent(Details->TakeWidget());
    DetailLayer->SetVisibility(EVisibility::Visible);
}

void UAgentsDlg::OpenIntroduction(FName Id)
{
    const auto* Brief=FactionBriefingCatalog::Find(Id);auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Brief || !Players || !FactionBriefingCatalog::Available(*Brief,Players->GetPlayerInfo()) || !DetailLayer)return;
    DetailLayer->SetContent(SNew(SBorder).Padding(18).BorderImage(&FlightPanelStyle::Window())
        [SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)
            [SNew(SHorizontalBox)+SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(Brief->Title.ToUpper())).Font(MissionUIStyle::GetHeaderFont(16))]
                +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ToolTipText(INVTEXT("Close briefing")).OnClicked_Lambda([this](){CloseDetails();return FReply::Handled();})[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Back)]]]
            +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[SNew(STextBlock).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(14)).Text(FText::FromString(Brief->Body))]]]);
    DetailLayer->SetVisibility(EVisibility::Visible);
    auto& Info=Players->GetMutablePlayerInfo();if(!Info.ViewedFactionBriefings.Contains(Id)){Info.ViewedFactionBriefings.Add(Id);Players->MarkDirty();}
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
    int32 OfferId=INDEX_NONE;
    Feedback=INVTEXT("No assignments are available for this agent and rank.");
    const bool Saved=CommitAgentCommand(Game,[&]()
    {
        if(!bCampaign)
        {
            FText Why;Catalog->PrepareIndependentOffer(SelectedAgent,Why);Eligible=Catalog->GetEligibleMissions(SelectedAgent);
            const bool HasDynamic=Eligible.ContainsByPredicate([](int32 Id){return DynamicMissionAI::IsDynamic(Id);});
            Eligible.RemoveAll([HasDynamic](int32 Id){return Id>=AgentMissionRules::CampaignBase || (HasDynamic && !DynamicMissionAI::IsDynamic(Id));});
            if(Eligible.IsEmpty() && !Why.IsEmpty())Feedback=Why;
        }
        const auto* Offer=Game->GetTrainingAgent().Request(PilotId(),Eligible,SelectedAgent,true,RegionalHistory(Game));
        if (Offer) OfferId=Offer->MissionId;
        return Offer!=nullptr;
    },Feedback);
    if (Saved) Feedback=FText::GetEmpty();
    RefreshBoard();
    if (Saved && OfferId!=INDEX_NONE) OpenDetails(OfferId,true);
}

void UAgentsDlg::NativeConstruct()
{
    AgentAI::OnJournalChanged().Remove(JournalChangedHandle);
    JournalChangedHandle=AgentAI::OnJournalChanged().AddWeakLambda(this,[this](){bJournalDirty=true;});
    Super::NativeConstruct();
    RefreshDirectory();
    RefreshBoard();
}

void UAgentsDlg::SelectDirectoryAgent()
{
    Feedback = FText::GetEmpty();
    CloseDetails();
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()); Game && SelectedAgent == TEXT("JanekFlightTraining"))
        if (auto* Catalog = Game->GetSubsystem<UAgentCatalogSubsystem>())
            CommitAgentCommand(Game,[&]() { return Game->GetTrainingAgent().Request(PilotId(),Catalog->GetEligibleMissions(SelectedAgent),SelectedAgent,true,RegionalHistory(Game))!=nullptr; },Feedback);
    RefreshBoard();
}

void UAgentsDlg::RefreshDirectory()
{
    if (bRefreshingDirectory) return;
    const FName PreviousAgent = SelectedAgent;
    {
        TGuardValue<bool> Guard(bRefreshingDirectory, true);
        auto* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
        auto* Environment = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStarshatterEnvironmentSubsystem>() : nullptr;
        auto* Simulation = Sim::GetSim();
        auto* Ship = Simulation ? Simulation->GetPlayerShip() : nullptr;
        auto* Region = Ship ? Ship->GetRegion() : Simulation ? Simulation->GetActiveRegion() : nullptr;
        auto* OriginSystem = Region ? Region->GetSystem() : nullptr;
        FName Origin = OriginSystem ? FName(UTF8_TO_TCHAR(OriginSystem->GetName())) : NAME_None;
        if (Origin.IsNone() && Simulation && Simulation->GetMission())
            if (const char* Name = Simulation->GetMission()->GetSystem()) Origin = FName(UTF8_TO_TCHAR(Name));
        if (Origin.IsNone() && GetGameInstance())
            if (auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>()) Origin = Players->GetPlayerInfo().HomeSystem;
        TSet<FName> Neighbors;
        if (Environment)
        {
            const auto& Systems = Environment->GetRuntimeStarSystems();
            if (!OriginSystem)
                for (auto* System : Systems)
                    if (System && FName(UTF8_TO_TCHAR(System->GetName())) == Origin) { OriginSystem = System; break; }
            if (OriginSystem)
                for (auto* System : Systems)
                    if (System && OriginSystem->HasLinkTo(System)) Neighbors.Add(FName(UTF8_TO_TCHAR(System->GetName())));
        }
        SystemChoices.Reset();
        if (Catalog)
            for (FName System : Catalog->GetDirectorySystems(Origin, Neighbors, DirectoryScope)) SystemChoices.Add(MakeShared<FName>(System));
        TSharedPtr<FName> SystemSelection;
        for (const auto& System : SystemChoices) if (*System == SelectedSystem) SystemSelection = System;
        if (!SystemSelection)
            for (const auto& System : SystemChoices) if (*System == Origin) SystemSelection = System;
        if (!SystemSelection && !SystemChoices.IsEmpty()) SystemSelection = SystemChoices[0];
        SelectedSystem = SystemSelection ? *SystemSelection : NAME_None;
        AgentChoices.Reset();
        if (Catalog)
            for (FName Agent : Catalog->GetSystemAgents(SelectedSystem)) AgentChoices.Add(MakeShared<FName>(Agent));
        TSharedPtr<FName> AgentSelection;
        for (const auto& Agent : AgentChoices) if (*Agent == SelectedAgent) AgentSelection = Agent;
        if (!AgentSelection)
            for (const auto& Agent : AgentChoices) if (*Agent == TEXT("JanekFlightTraining")) AgentSelection = Agent;
        if (!AgentSelection && !AgentChoices.IsEmpty()) AgentSelection = AgentChoices[0];
        SelectedAgent = AgentSelection ? *AgentSelection : NAME_None;
        if (SystemPicker) { SystemPicker->RefreshOptions(); SystemPicker->SetSelectedItem(SystemSelection); }
        if (AgentPicker) { AgentPicker->RefreshOptions(); AgentPicker->SetSelectedItem(AgentSelection); }
    }
    if (PreviousAgent != SelectedAgent) SelectDirectoryAgent();
}

TSharedRef<SWidget> UAgentsDlg::RebuildWidget()
{
    SetIsFocusable(true);
    auto* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    SystemPicker.Reset(); AgentPicker.Reset();
    RefreshDirectory();
    TSharedPtr<FName> InitialSystem, InitialAgent;
    for (const auto& Item : SystemChoices) if (*Item == SelectedSystem) InitialSystem = Item;
    for (const auto& Item : AgentChoices) if (*Item == SelectedAgent) InitialAgent = Item;

    const auto AgentLabel = [Catalog](FName Id)
    {
        const auto* Agent = Catalog ? Catalog->FindAgent(Id) : nullptr;
        return Agent ? FText::FromString(FString::Printf(TEXT("%s   /   LEVEL %d"), *Agent->DisplayName, Agent->Level))
            : INVTEXT("No neutral or own-empire agents");
    };
    const auto Content = SNew(SOverlay)
        + SOverlay::Slot()
        [
            MissionUIStyle::MakeDataPane(
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,4,0)
                    [SNew(SButton).Text(INVTEXT("SAME SYSTEM"))
                        .ToolTipText(INVTEXT("Show neutral and own-empire agents in your current system"))
                        .ButtonColorAndOpacity_Lambda([this]() { return DirectoryScope == EAgentDirectoryScope::SameSystem ? FLinearColor(.08f,.35f,.45f) : FLinearColor(.08f,.12f,.16f); })
                        .OnClicked_Lambda([this]() { DirectoryScope=EAgentDirectoryScope::SameSystem; RefreshDirectory(); return FReply::Handled(); })]
                    + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,4,0)
                    [SNew(SButton).Text(INVTEXT("ONE JUMP"))
                        .ToolTipText(INVTEXT("Show neutral and own-empire agents here and one farcaster link away"))
                        .ButtonColorAndOpacity_Lambda([this]() { return DirectoryScope == EAgentDirectoryScope::OneJump ? FLinearColor(.08f,.35f,.45f) : FLinearColor(.08f,.12f,.16f); })
                        .OnClicked_Lambda([this]() { DirectoryScope=EAgentDirectoryScope::OneJump; RefreshDirectory(); return FReply::Handled(); })]
                    + SHorizontalBox::Slot().FillWidth(1)
                    [SNew(SButton).Text(INVTEXT("EMPIRE-WIDE"))
                        .ToolTipText(INVTEXT("Show neutral and own-empire agents throughout the galaxy"))
                        .ButtonColorAndOpacity_Lambda([this]() { return DirectoryScope == EAgentDirectoryScope::EmpireWide ? FLinearColor(.08f,.35f,.45f) : FLinearColor(.08f,.12f,.16f); })
                        .OnClicked_Lambda([this]() { DirectoryScope=EAgentDirectoryScope::EmpireWide; RefreshDirectory(); return FReply::Handled(); })]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1).Padding(0,0,8,0)
                    [SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("SYSTEM")).Font(MissionUIStyle::GetRowFont(12))]
                        + SVerticalBox::Slot().AutoHeight()
                        [SAssignNew(SystemPicker,SComboBox<TSharedPtr<FName>>).OptionsSource(&SystemChoices).InitiallySelectedItem(InitialSystem)
                            .OnGenerateWidget_Lambda([](TSharedPtr<FName> Item) { return SNew(STextBlock).Text(FText::FromName(*Item).ToUpper()).Font(MissionUIStyle::GetRowFont(14)); })
                            .OnSelectionChanged_Lambda([this](TSharedPtr<FName> Item,ESelectInfo::Type)
                            { if (bRefreshingDirectory) return; SelectedSystem=Item ? *Item : NAME_None; RefreshDirectory(); })
                            [SNew(STextBlock).Text_Lambda([this]() { return SelectedSystem.IsNone() ? INVTEXT("NO SYSTEMS IN RANGE") : FText::FromName(SelectedSystem).ToUpper(); }).Font(MissionUIStyle::GetHeaderFont(15))]]]
                    + SHorizontalBox::Slot().FillWidth(1.5f)
                    [SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(INVTEXT("AGENT")).Font(MissionUIStyle::GetRowFont(12))]
                        + SVerticalBox::Slot().AutoHeight()
                        [SAssignNew(AgentPicker,SComboBox<TSharedPtr<FName>>).OptionsSource(&AgentChoices).InitiallySelectedItem(InitialAgent)
                            .IsEnabled_Lambda([this]() { return !SelectedSystem.IsNone() && !AgentChoices.IsEmpty(); })
                            .OnGenerateWidget_Lambda([AgentLabel](TSharedPtr<FName> Item) { return SNew(STextBlock).Text(AgentLabel(*Item)).Font(MissionUIStyle::GetRowFont(14)); })
                            .OnSelectionChanged_Lambda([this](TSharedPtr<FName> Item,ESelectInfo::Type)
                            { if (bRefreshingDirectory) return; SelectedAgent=Item ? *Item : NAME_None; SelectDirectoryAgent(); })
                            [SNew(STextBlock).Text_Lambda([this,AgentLabel]() { return AgentLabel(SelectedAgent); }).Font(MissionUIStyle::GetHeaderFont(15))]]]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 12, 12)
                [SNew(SRecordDetails).EventDriven(true).SelectionRevision_Lambda([this](){return GetTypeHash(SelectedAgent);}).Record_Lambda([this, Catalog]()
                    {
                        FRecordDetails D;D.Icon=EShipSystemIcon::Agent;
                        const auto* Agent=Catalog?Catalog->FindAgent(SelectedAgent):nullptr;
                        if(!Agent){D.Title=INVTEXT("SELECT AN AGENT");return D;}
                        D.Title=FText::FromString(Agent->DisplayName);D.Subtitle=FText::FromString(Agent->Division);D.Description=FText::FromString(Agent->Description);
                        D.Fact(INVTEXT("LEVEL"),FText::AsNumber(Agent->Level)).Fact(INVTEXT("PLANET"),FText::FromString(Agent->Planet)).Fact(INVTEXT("STATION"),FText::FromString(Agent->Station));return D;
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
        CommitAgentCommand(Game,[&]() { return Game->GetTrainingAgent().Request(PilotId(),Eligible,SelectedAgent,true,RegionalHistory(Game))!=nullptr; },Feedback);
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
    SystemPicker.Reset(); AgentPicker.Reset();
    DetailLayer.Reset();
}

void UAgentsDlg::NativeDestruct()
{
    AgentAI::OnJournalChanged().Remove(JournalChangedHandle);JournalChangedHandle.Reset();
    CloseDetails();Super::NativeDestruct();
}
void UAgentsDlg::NativeTick(const FGeometry& Geometry,float Delta)
{
    Super::NativeTick(Geometry,Delta);
    if(bJournalDirty){CloseDetails();RefreshBoard();}
}
