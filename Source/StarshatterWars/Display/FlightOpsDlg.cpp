/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         FlightOpsDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Flight Operations Screen (UMG / UUserWidget implementation)
*/

#include "FlightOpsDlg.h"
#include "ShipInsuranceSubsystem.h"
#include "Engine/GameInstance.h"
#include "FlightOpsPopup.h"
#include "FlightOpsAccessRules.h"
#include "SimElement.h"
#include "Engine/World.h"

#include "GameStructs.h"

// Unreal:
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"

// Starshatter:
#include "Ship.h"
#include "ShipDesign.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "FlightPlanner.h"
#include "Mission.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Sim.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlightOpsDlg, Log, All);

// +--------------------------------------------------------------------+

UFlightOpsDlg::UFlightOpsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    MissionButtons[0] = MissionButtonPatrol;
    MissionButtons[1] = MissionButtonIntercept;
    MissionButtons[2] = MissionButtonAssault;
    MissionButtons[3] = MissionButtonStrike;
    MissionButtons[4] = MissionButtonEscort;
    MissionButtons[5] = MissionButtonScout;

    if (FilterList)
    {
        FilterList->OnSelectionChanged.AddDynamic(
            this, &UFlightOpsDlg::OnFilterChanged);
    }

    if (PackageButton)  PackageButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnPackageClicked);
    if (AlertButton)    AlertButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnAlertClicked);
    if (LaunchButton)   LaunchButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnLaunchClicked);
    if (StandbyButton)  StandbyButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnStandDownClicked);
    if (RecallButton)   RecallButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnRecallClicked);
    if (CloseButton)    CloseButton->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnCloseClicked);

    if (MissionButtonPatrol)     MissionButtonPatrol->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionPatrolClicked);
    if (MissionButtonIntercept)  MissionButtonIntercept->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionInterceptClicked);
    if (MissionButtonAssault)    MissionButtonAssault->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionAssaultClicked);
    if (MissionButtonStrike)     MissionButtonStrike->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionStrikeClicked);
    if (MissionButtonEscort)     MissionButtonEscort->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionEscortClicked);
    if (MissionButtonScout)      MissionButtonScout->OnClicked.AddDynamic(this, &UFlightOpsDlg::OnMissionScoutClicked);
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::NativeDestruct()
{
    RuntimePanel.Reset();
    delete FlightPlannerPtr;
    FlightPlannerPtr = nullptr;

    Super::NativeDestruct();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (HangarList && !RuntimePanel.IsValid()) ExecFrame();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::SetShip(Ship* InShip)
{
    if (ShipPtr == InShip)
        return;

    ShipPtr = InShip;

    delete FlightPlannerPtr;
    FlightPlannerPtr = nullptr;

    if (!FilterList)
        return;

    FilterList->ClearOptions();

    if (!ShipPtr)
        return;

    Hangar* HangarPtr = ShipPtr->GetHangar();
    if (!HangarPtr)
        return;

    const int32 NumSquadrons = HangarPtr->NumSquadrons();
    for (int32 i = 0; i < NumSquadrons; ++i)
    {
        const FString Abbrev = ANSI_TO_TCHAR(HangarPtr->SquadronDesign(i)->abrv);
        const FString SqnName = ANSI_TO_TCHAR(HangarPtr->SquadronName(i).data());

        FilterList->AddOption(FString::Printf(TEXT("%s %s"), *Abbrev, *SqnName));
    }

    FilterList->AddOption(TEXT("Pending"));
    FilterList->AddOption(TEXT("Active"));

    FlightPlannerPtr = new FlightPlanner(ShipPtr);

    FilterList->SetSelectedIndex(0);
    RefreshHangarListForFilterIndex(0);
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::ExecFrame()
{
    if (!ShipPtr || !ShipPtr->GetHangar())
    {
        UE_LOG(LogFlightOpsDlg, Warning, TEXT("FlightOpsDlg: Ship or Hangar invalid"));
        return;
    }

    UpdateSelection();
    UpdateObjective();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::UpdateSelection()
{
    // NOTE:
    // Full fidelity logic mirrors the original FltDlg implementation.
    // This method intentionally remains logic-only; ListView population
    // is delegated to RefreshHangarListForFilterIndex().

    const bool Allowed=CanIssueOrders();
    for(UButton* Button:{PackageButton,AlertButton,LaunchButton,StandbyButton,RecallButton,MissionButtonPatrol,MissionButtonIntercept,MissionButtonAssault,MissionButtonStrike,MissionButtonEscort,MissionButtonScout})
        if(Button)Button->SetIsEnabled(Allowed);
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::UpdateObjective()
{
    if (!ObjectiveList || MissionType < 0 || !ShipPtr)
        return;

    Sim* SimPtr = Sim::GetSim();
    if (!SimPtr)
        return;

    // Objective refresh mirrors classic logic but is intentionally
    // deferred to UListView row objects.
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::RefreshHangarListForFilterIndex(int32 FilterIndex)
{
    if (!HangarList || !ShipPtr)
        return;

    HangarList->ClearListItems();

    Hangar* HangarPtr = ShipPtr->GetHangar();
    if (!HangarPtr)
        return;

    // NOTE:
    // Each row should be backed by a UObject list item representing
    // the classic hangar slot state (index, name, status, mission, time).
    // That UObject type is intentionally left decoupled.
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::SetMissionType(int32 InMissionType)
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) { RuntimePanel->ChooseMission(InMissionType); return; }
    MissionType = InMissionType;

    for (int32 i = 0; i < 6; ++i)
    {
        if (MissionButtons[i])
        {
            MissionButtons[i]->SetIsEnabled(i == MissionType);
        }
    }

    UpdateObjective();
}

// +--------------------------------------------------------------------+
// UI EVENTS
// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnFilterChanged(FString SelectedItem, ESelectInfo::Type)
{
    if (!FilterList)
        return;

    const int32 Index = FilterList->GetSelectedIndex();
    if (RuntimePanel.IsValid()) RuntimePanel->ChooseFilter(Index);
    else RefreshHangarListForFilterIndex(Index);
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnPackageClicked()
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) RuntimePanel->PreparePackage();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnAlertClicked()
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) RuntimePanel->PrepareAlert();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnLaunchClicked()
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) RuntimePanel->LaunchSelected();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnStandDownClicked()
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) RuntimePanel->StandDownSelected();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnRecallClicked()
{
    if(!CanIssueOrders())return;
    if (RuntimePanel.IsValid()) RuntimePanel->RecallSelected();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnCloseClicked()
{
    RequestPanelClose();
}

// +--------------------------------------------------------------------+
// Mission buttons
// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnMissionPatrolClicked() { SetMissionType(0); }
void UFlightOpsDlg::OnMissionInterceptClicked() { SetMissionType(1); }
void UFlightOpsDlg::OnMissionAssaultClicked() { SetMissionType(2); }
void UFlightOpsDlg::OnMissionStrikeClicked() { SetMissionType(3); }
void UFlightOpsDlg::OnMissionEscortClicked() { SetMissionType(4); }
void UFlightOpsDlg::OnMissionScoutClicked() { SetMissionType(5); }

bool UFlightOpsDlg::CanIssueOrders() const
{
    auto* Simulation=Sim::GetSim();
    return GetWorld() && !GetWorld()->IsPaused() && Simulation && Simulation->HasSimulationAuthority() &&
        FlightOpsAccessRules::CanCommand(ResolvePanelShip(),ResolveCarrier());
}
Ship* UFlightOpsDlg::ResolveCarrier() const
{
    Ship* Player=ResolvePanelShip();
    if(!Player)return nullptr;
    if(Player->GetHangar())return Player;
    SimElement* Element=Player->GetElement();
    Ship* Carrier=Element?Element->GetCarrier():nullptr;
    return Carrier && !Carrier->IsDead()?Carrier:nullptr;
}
TSharedRef<SWidget> UFlightOpsDlg::CreatePanelContent()
{
    if (auto* GI=GetGameInstance()) if(auto* Sales=GI->GetSubsystem<UShipInsuranceSubsystem>())
    { FText Reason; Sales->PrepareCarrierFighterSales(Reason); }
    if (PanelTitle) PanelTitle->SetVisibility(ESlateVisibility::Collapsed);
    const TWeakObjectPtr<UFlightOpsDlg> Owner(this);
    return SAssignNew(RuntimePanel,SFlightOpsPopup)
        .ResolveCarrier([Owner](){return Owner.IsValid()?Owner->ResolveCarrier():nullptr;})
        .CanOperate([Owner](){return Owner.IsValid() && Owner->CanIssueOrders();});
}
FText UFlightOpsDlg::GetPanelCaption() const{return FText::FromString(TEXT("FLIGHT OPERATIONS"));}
