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

#include "GameStructs.h"

// Unreal:
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
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
    delete FlightPlannerPtr;
    FlightPlannerPtr = nullptr;

    Super::NativeDestruct();
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame();
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

    // Button enable/disable logic will be added here as UMG bindings mature.
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
    RefreshHangarListForFilterIndex(Index);
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnPackageClicked()
{
    UE_LOG(LogFlightOpsDlg, Log, TEXT("Package clicked"));
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnAlertClicked()
{
    UE_LOG(LogFlightOpsDlg, Log, TEXT("Alert clicked"));
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnLaunchClicked()
{
    UE_LOG(LogFlightOpsDlg, Log, TEXT("Launch clicked"));
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnStandDownClicked()
{
    UE_LOG(LogFlightOpsDlg, Log, TEXT("Stand Down clicked"));
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnRecallClicked()
{
    UE_LOG(LogFlightOpsDlg, Log, TEXT("Recall clicked"));
}

// +--------------------------------------------------------------------+

void UFlightOpsDlg::OnCloseClicked()
{
    RemoveFromParent();
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
