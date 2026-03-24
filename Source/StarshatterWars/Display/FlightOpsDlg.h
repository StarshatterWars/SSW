/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         FlightOpsDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Flight Operations Screen (Unreal UMG port of classic FltDlg)

    NOTES
    =====
    - UUserWidget-based (UMG) and intended to inherit from your UBaseScreen.
    - Keeps Starshatter core types (Text, List, etc.) as plain C++ wherever applicable.
    - Minimal Unreal includes included only where needed.
    - Element* renamed to SimElement* (simulation-side).
*/

#pragma once

#include "Math/Vector.h"             // FVector
#include "Math/Color.h"              // FColor
#include "Math/UnrealMathUtility.h"  // FMath
#include "BaseScreen.h"

#include "Blueprint/UserWidget.h"
#include "FlightOpsDlg.generated.h"

// +--------------------------------------------------------------------+
// Forward Declarations (Unreal):

class UButton;
class UComboBoxString;
class UListView;

// +--------------------------------------------------------------------+
// Forward Declarations (Starshatter / game sim):

class Ship;
class ShipDesign;
class Hangar;
class FlightDeck;
class FlightPlanner;
class Sim;
class SimElement;
class Instruction;

// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UFlightOpsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UFlightOpsDlg(const FObjectInitializer& ObjectInitializer);

    // UUserWidget:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    void SetShip(Ship* InShip);
    void ExecFrame();

protected:
    // -----------------------------------------------------------------
    // UI Wiring (BindWidget):

    UPROPERTY(meta = (BindWidgetOptional))
    UComboBoxString* FilterList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* HangarList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* PackageButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* AlertButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* LaunchButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* StandbyButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RecallButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* CloseButton = nullptr;

    // Mission type buttons:
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonPatrol = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonIntercept = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonAssault = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonStrike = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonEscort = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* MissionButtonScout = nullptr;

    // Objective / Loadout lists:
    UPROPERTY(meta = (BindWidgetOptional))
    UListView* ObjectiveList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* LoadoutList = nullptr;

    // -----------------------------------------------------------------
    // State:

    Ship* ShipPtr = nullptr;
    FlightPlanner* FlightPlannerPtr = nullptr;

    int32 MissionType = -1;
    int32 PatrolPattern = 0;

    UButton* MissionButtons[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

    // -----------------------------------------------------------------
    // Internal helpers:

    void UpdateSelection();
    void UpdateObjective();

    void RefreshHangarListForFilterIndex(int32 FilterIndex);
    void SetMissionType(int32 InMissionType);

    // -----------------------------------------------------------------
    // UI Events (UMG):

    UFUNCTION()
    void OnFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnPackageClicked();

    UFUNCTION()
    void OnAlertClicked();

    UFUNCTION()
    void OnLaunchClicked();

    UFUNCTION()
    void OnStandDownClicked();

    UFUNCTION()
    void OnRecallClicked();

    UFUNCTION()
    void OnCloseClicked();

    // Mission selection handlers:
    UFUNCTION()
    void OnMissionPatrolClicked();

    UFUNCTION()
    void OnMissionInterceptClicked();

    UFUNCTION()
    void OnMissionAssaultClicked();

    UFUNCTION()
    void OnMissionStrikeClicked();

    UFUNCTION()
    void OnMissionEscortClicked();

    UFUNCTION()
    void OnMissionScoutClicked();
};
