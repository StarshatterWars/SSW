#include "AgentsDlg.h"
#include "PanelCloseIcon.h"
#include "StarshatterPlayerSubsystem.h"

#include "AgentAI.h"
#include "AgentCatalogSubsystem.h"
#include "Widgets/Input/SComboBox.h"
#include "TimerManager.h"
#include "Widgets/SNullWidget.h"
#include "MenuScreen.h"
#include "Mission.h"
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
                    return CanLaunch.IsBound() && CanLaunch.Execute() ? INVTEXT("Open briefing for the next queued mission")
                        : INVTEXT("Dock before starting the next queued mission");
                })
                .Visibility(bHead ? EVisibility::Visible : EVisibility::Collapsed)
                .IsEnabled_Lambda([this]() { return CanLaunch.IsBound() && CanLaunch.Execute(); })
                .OnClicked(FOnClicked::CreateWeakLambda(this, [this, Id]()
                {
                    if (CanLaunch.IsBound() && CanLaunch.Execute())
                        if (auto* Menu = GetMenuManager())
                        {
                            // A live scene owns this widget. Let it finish the click before replacing the scene.
                            if (auto* World = GetWorld())
                                World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Menu, Id]()
                                {
                                    if (!CanLaunch.IsBound() || !CanLaunch.Execute()) return;
                                    if (bStandaloneFrame) RemoveFromParent();
                                    Menu->OpenQueuedAssignment(Id, !bStandaloneFrame);
                                }));
                        }
                    return FReply::Handled();
                }))[SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]]
        ];
    };

    if (const auto* Offer = Agent.FindOffer(PilotId(), SelectedAgent))
        AddMission(Offer->MissionId, INVTEXT("OFFERED"), true, false);
    bool bHead = true;
    for (const auto& Mission : Agent.GetQueue(PilotId()))
    {
        AddMission(Mission.MissionId, Mission.State == State::InProgress ? INVTEXT("IN PROGRESS") : INVTEXT("QUEUED"),
            false, bHead && Mission.State == State::Accepted);
        bHead = false;
    }
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
            if (!Catalog || !Catalog->CanAccept(MissionId, Issuer) ||
                !Instance->GetTrainingAgent().Accept(PilotId(), MissionId)) return;
            if (auto* Players = Instance->GetSubsystem<UStarshatterPlayerSubsystem>()) Players->SavePlayer(true);
            CloseDetails();
            RefreshBoard();
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

    FName Issuer=SelectedAgent; FText Status=bOffer ? INVTEXT("OFFERED") : INVTEXT("QUEUED");
    for (const auto& Assignment : Game->GetTrainingAgent().GetQueue(PilotId()))
        if (Assignment.MissionId==MissionId) { Issuer=Assignment.Issuer; if(Assignment.State==State::InProgress)Status=INVTEXT("IN PROGRESS"); break; }
    const auto* Officer=Catalog->FindAgent(Issuer);
    CastChecked<UMissionObjectivesDlg>(Details)->SetMissionBriefing(*Mission,Catalog->GetMissionTitle(MissionId),
        Officer ? FText::FromString(Officer->DisplayName) : INVTEXT("Mission Control"),Status,
        MoveTemp(Accept),MoveTemp(Decline),FSimpleDelegate::CreateUObject(this,&UAgentsDlg::CloseDetails));
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

void UAgentsDlg::RequestMission()
{
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    auto* Catalog = Game ? Game->GetSubsystem<UAgentCatalogSubsystem>() : nullptr;
    if (!Catalog || !Catalog->CanContact(SelectedAgent)) return;
    const auto Eligible = Catalog->GetEligibleMissions(SelectedAgent);
    const auto* Offer = Game->GetTrainingAgent().Request(PilotId(), Eligible, SelectedAgent);
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
        + SOverlay::Slot()
        [SAssignNew(DetailLayer, SBox).Visibility(EVisibility::Collapsed)];
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
