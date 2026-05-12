/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionDebriefDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Debriefing dialog (Unreal) - UBaseScreen/UMG port
*/

#include "MissionDebriefDlg.h"

#include "MissionPlanner.h"   // your renamed PlanScreen (do not rename)
#include "Starshatter.h"
#include "Campaign.h"
#include "Mission.h"
#include "Sim.h"
#include "Ship.h"
#include "StarSystem.h"
#include "SimElement.h"
#include "Instruction.h"
#include "FormatUtil.h"
#include "PlayerCharacter.h"
#include "SimEvent.h"
#include "ShipDesign.h"
#include "GameStructs.h"
#include "SSWRuntimeSubsystem.h"


// Legacy helpers:
#include "Game.h"
#include "Mouse.h"

// Unreal:
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"

UMissionDebriefDlg::UMissionDebriefDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        MissionPtr = CampaignPtr->GetMission();
    }
}

void UMissionDebriefDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind ID -> widget mapping into UBaseScreen’s FormMap:
    BindFormWidgets();

    // Button click:
    if (CloseButtonWidget)
    {
        CloseButtonWidget->OnClicked.AddDynamic(this, &UMissionDebriefDlg::OnCloseClicked);
    }

    // Unit selection:
    if (UnitListWidget)
    {
        UnitListWidget->OnItemSelectionChanged().AddUObject(this, &UMissionDebriefDlg::OnUnitSelectionChanged);
    }

    // Default hidden unless you show it:
    // (Optional) leave as-is if your UI manages visibility elsewhere.
    // SetVisibility(ESlateVisibility::Collapsed);
}

void UMissionDebriefDlg::NativeDestruct()
{
    Super::NativeDestruct();
}

void UMissionDebriefDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsShown)
    {
        ExecFrame(InDeltaTime);
    }
}

// --------------------------------------------------------------------
// FORM binding (IDs -> widgets)
// --------------------------------------------------------------------

void UMissionDebriefDlg::BindFormWidgets()
{
    // Clear the map (optional safety):
    // FormMap.Reset(); // FormMap is protected in UBaseScreen; if you keep it private, remove this.

    // IDs come from legacy DebriefDlg.frm mapping:
    if (MissionNameWidget)      BindLabel(200, MissionNameWidget);
    if (MissionSystemWidget)    BindLabel(202, MissionSystemWidget);
    if (MissionSectorWidget)    BindLabel(204, MissionSectorWidget);
    if (MissionTimeStartWidget) BindLabel(206, MissionTimeStartWidget);

    if (MissionScoreWidget)     BindLabel(211, MissionScoreWidget);

    if (ObjectivesWidget)       BindText(210, ObjectivesWidget);
    if (SituationWidget)        BindText(240, SituationWidget);

    if (CloseButtonWidget)      BindButton(1, CloseButtonWidget);

    if (UnitListWidget)         BindList(320, UnitListWidget);
    if (SummaryListWidget)      BindList(330, SummaryListWidget);
    if (EventListWidget)        BindList(340, EventListWidget);
}

// --------------------------------------------------------------------
// Dialog input hooks (Enter/Escape)
// --------------------------------------------------------------------

void UMissionDebriefDlg::HandleAccept()
{
    // Legacy: Enter closes
    OnCloseClicked();
}

void UMissionDebriefDlg::HandleCancel()
{
    // Escape closes as well for debrief:
    OnCloseClicked();
}

// --------------------------------------------------------------------
// Show / Hide (UMG)
// --------------------------------------------------------------------

void UMissionDebriefDlg::Show()
{
    const bool bNeedRefresh = (GetVisibility() != ESlateVisibility::Visible);

    SetVisibility(ESlateVisibility::Visible);
    bIsShown = true;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (USSWRuntimeSubsystem* RuntimeSS = GI->GetSubsystem<USSWRuntimeSubsystem>())
        {
            RuntimeSS->SetTimeCompression(1);
        }
    }

    MissionPtr = nullptr;
    CampaignPtr = Campaign::GetCampaign();
    SimPtr = Sim::GetSim();

    PlayerShip = nullptr;
    if (SimPtr)
    {
        PlayerShip = SimPtr->GetPlayerShip();
    }

    if (CampaignPtr)
    {
        MissionPtr = CampaignPtr->GetMission();
    }

    // Mission title:
    if (UTextBlock* MissionName = GetLabel(200))
    {
        if (MissionPtr)
            MissionName->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetName())));
        else
            MissionName->SetText(FText::FromString(ANSI_TO_TCHAR(Game::GetText("DebriefDlg.mission-name").data())));
    }

    // System:
    if (UTextBlock* MissionSystem = GetLabel(202))
    {
        MissionSystem->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            StarSystem* Sys = MissionPtr->GetStarSystem();
            if (Sys)
                MissionSystem->SetText(FText::FromString(ANSI_TO_TCHAR(Sys->GetName())));
        }
    }

    // Sector/Region:
    if (UTextBlock* MissionSector = GetLabel(204))
    {
        MissionSector->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            MissionElement* Elem0 = MissionPtr->GetElements().size() ? MissionPtr->GetElements().at(0) : nullptr;
            if (Elem0)
                MissionSector->SetText(FText::FromString(ANSI_TO_TCHAR(Elem0->GetRegion())));
        }
    }

    // Start time:
    if (UTextBlock* MissionTimeStart = GetLabel(206))
    {
        MissionTimeStart->SetText(FText::GetEmpty());

        if (MissionPtr)
        {
            char Txt[32] = { 0 };
            FormatDayTime(Txt, MissionPtr->GetStart());
            MissionTimeStart->SetText(FText::FromString(ANSI_TO_TCHAR(Txt)));
        }
    }

    // Objectives:
    if (URichTextBlock* Objectives = GetText(210))
    {
        bool bFoundObjectives = false;

        if (SimPtr && SimPtr->GetPlayerElement())
        {
            Text T;
            SimElement* PlayerElem = SimPtr->GetPlayerElement();

            for (int i = 0; i < PlayerElem->NumObjectives(); i++)
            {
                Instruction* Obj = PlayerElem->GetObjective(i);
                T += Text("* ") + Obj->GetDescription() + Text("\n");
                bFoundObjectives = true;
            }

            Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(T.data())));
        }

        if (!bFoundObjectives)
        {
            if (MissionPtr)
                Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetObjective())));
            else
                Objectives->SetText(FText::FromString(ANSI_TO_TCHAR(Game::GetText("DebriefDlg.unspecified").data())));
        }
    }

    // Situation:
    if (URichTextBlock* Situation = GetText(240))
    {
        if (MissionPtr)
            Situation->SetText(FText::FromString(ANSI_TO_TCHAR(MissionPtr->GetSituation())));
        else
            Situation->SetText(FText::FromString(ANSI_TO_TCHAR(Game::GetText("DebriefDlg.unknown").data())));
    }

    // Score:
    if (UTextBlock* MissionScore = GetLabel(211))
    {
        MissionScore->SetText(FText::FromString(ANSI_TO_TCHAR(Game::GetText("DebriefDlg.no-stats").data())));

        if (PlayerShip)
        {
            for (int i = 0; i < ShipStats::NumStats(); i++)
            {
                ShipStats* Stats = ShipStats::GetStats(i);
                if (Stats && !strcmp(PlayerShip->GetName(), Stats->GetName()))
                {
                    Stats->Summarize();

                    PlayerCharacter* PlayerObj = PlayerCharacter::GetCurrentPlayer();
                    int Points = Stats->GetPoints() + Stats->GetCommandPoints();

                    if (PlayerObj && SimPtr)
                        Points = PlayerObj->GetMissionPoints(Stats, SimPtr->StartTime()) + Stats->GetCommandPoints();

                    char ScoreTxt[64] = { 0 };
                    sprintf_s(ScoreTxt, "%d %s", Points, Game::GetText("DebriefDlg.points").data());
                    MissionScore->SetText(FText::FromString(ANSI_TO_TCHAR(ScoreTxt)));
                    break;
                }
            }
        }
    }

    // Lists:
    DrawUnits();

    if (bNeedRefresh)
    {
        // Optional: force selection behavior or first row selection.
        // (Handled in ExecFrame)
    }
}

void UMissionDebriefDlg::Hide()
{
    bIsShown = false;
    SetVisibility(ESlateVisibility::Collapsed);
}

// --------------------------------------------------------------------
// Frame
// --------------------------------------------------------------------

void UMissionDebriefDlg::ExecFrame(float /*DeltaSeconds*/)
{
    // Legacy: if list has items but none selected, force select first
    if (UListView* UnitList = GetList(320))
    {
        if (UnitList->GetNumItems() > 0)
        {
            TArray<UObject*> Selected;
            UnitList->GetSelectedItems(Selected);

            if (Selected.Num() < 1)
            {
                UnitList->SetSelectedIndex(0);
            }
        }
    }
}

// --------------------------------------------------------------------
// Units + selection
// --------------------------------------------------------------------

void UMissionDebriefDlg::DrawUnits()
{
    if (!MissionPtr)
        return;

    UListView* UnitList = GetList(320);
    if (!UnitList)
        return;

    UnitList->ClearListItems();

    int32 SelectIndex = INDEX_NONE;

    for (int i = 0; i < ShipStats::NumStats(); i++)
    {
        ShipStats* Stats = ShipStats::GetStats(i);
        if (!Stats) continue;

        Stats->Summarize();

        if (Stats->GetIFF() == MissionPtr->GetTeam() &&
            !strcmp(Stats->GetRegion(), MissionPtr->GetRegion()))
        {
            // Col0 unused in original (space), keep for compatibility:
            UDebriefListItem* Row = UDebriefListItem::Make(
                this,
                TEXT(" "),
                ANSI_TO_TCHAR(Stats->GetName()),
                ANSI_TO_TCHAR(Stats->GetRole()),
                ANSI_TO_TCHAR(Stats->GetType()),
                /*DataIndex*/ i
            );

            UnitList->AddItem(Row);

            if (PlayerShip && !strcmp(PlayerShip->GetName(), Stats->GetName()))
            {
                SelectIndex = UnitList->GetNumItems() - 1;
            }
        }
    }

    if (SelectIndex != INDEX_NONE)
    {
        UnitList->SetSelectedIndex(SelectIndex);
    }
}

void UMissionDebriefDlg::OnUnitSelectionChanged(UObject* SelectedItem)
{
    const UDebriefListItem* Item = Cast<UDebriefListItem>(SelectedItem);
    if (!Item)
        return;

    DrawSelectedUnit(Item->DataIndex);
}

void UMissionDebriefDlg::DrawSelectedUnit(int32 StatsIndex)
{
    UListView* SummaryList = GetList(330);
    UListView* EventList = GetList(340);

    if (!SummaryList || !EventList)
        return;

    SummaryList->ClearListItems();
    EventList->ClearListItems();

    ShipStats* Stats = (StatsIndex >= 0) ? ShipStats::GetStats(StatsIndex) : nullptr;
    if (!Stats)
        return;

    Stats->Summarize();

    // Summary:
    auto AddSummary = [&](const char* Label, int Value)
        {
            char Buf[64] = { 0 };
            sprintf_s(Buf, "%d", Value);

            SummaryList->AddItem(UDebriefListItem::Make(
                this,
                ANSI_TO_TCHAR(Label),
                ANSI_TO_TCHAR(Buf)
            ));
        };

    AddSummary("Guns Fired:", Stats->GetGunShots());
    AddSummary("Gun Hits:", Stats->GetGunHits());
    AddSummary("Gun Kills:", Stats->GetGunKills());

    SummaryList->AddItem(UDebriefListItem::Make(this, TEXT(" "))); // spacer

    AddSummary("Missiles Fired:", Stats->GetMissileShots());
    AddSummary("Missile Hits:", Stats->GetMissileHits());
    AddSummary("Missile Kills:", Stats->GetMissileKills());

    // Events:
    ListIter<SimEvent> Iter = Stats->GetEvents();
    while (++Iter)
    {
        SimEvent* E = Iter.value();
        if (!E) continue;

        char TimeTxt[64] = { 0 };
        const int Time = E->GetTime();

        if (Time > 24 * 60 * 60) FormatDayTime(TimeTxt, Time);
        else                     FormatTime(TimeTxt, Time);

        const char* Target = E->GetTarget();

        EventList->AddItem(UDebriefListItem::Make(
            this,
            ANSI_TO_TCHAR(TimeTxt),
            ANSI_TO_TCHAR(E->GetEventDesc()),
            Target ? ANSI_TO_TCHAR(Target) : TEXT("")
        ));
    }
}

// --------------------------------------------------------------------
// Close
// --------------------------------------------------------------------

void UMissionDebriefDlg::OnCloseClicked()
{
    Sim* LocalSim = Sim::GetSim();
    if (!LocalSim)
    {
        UE_LOG(LogTemp, Error,
            TEXT("UMissionDebriefDlg::OnCloseClicked - Sim::GetSim returned nullptr"));
        return;
    }

    LocalSim->CommitMission();
    LocalSim->UnloadMission();

    PlayerCharacter* PlayerObj = PlayerCharacter::GetCurrentPlayer();
    if (PlayerObj && PlayerObj->ShowAward())
    {
        if (Manager)
        {
            Manager->ShowAwardDlg();
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("UMissionDebriefDlg::OnCloseClicked - Manager is nullptr while attempting to show award dialog"));
        }

        return;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Error,
            TEXT("UMissionDebriefDlg::OnCloseClicked - GameInstance not found"));
        return;
    }

    USSWRuntimeSubsystem* RuntimeSS =
        GI->GetSubsystem<USSWRuntimeSubsystem>();

    if (!RuntimeSS)
    {
        UE_LOG(LogTemp, Error,
            TEXT("UMissionDebriefDlg::OnCloseClicked - Runtime subsystem not found"));
        return;
    }

    Mouse::Show(false);

    Campaign* Camp = Campaign::GetCampaign();

    if (Camp && Camp->GetCampaignId() < Campaign::SINGLE_MISSIONS)
    {
        UE_LOG(LogTemp, Log,
            TEXT("UMissionDebriefDlg::OnCloseClicked - Returning to Campaign Mode"));

        RuntimeSS->SetGameMode(EGameMode::CMPN);
    }
    else
    {
        UE_LOG(LogTemp, Log,
            TEXT("UMissionDebriefDlg::OnCloseClicked - Returning to Main Menu"));

        RuntimeSS->SetGameMode(EGameMode::MENU);
    }
}