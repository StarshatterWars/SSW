/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission weapon dialog.

    Displays available preset loadouts for the player ship and
    displays the currently selected runtime MissionLoad stations.

    NOTES
    =====
    - Preset loadouts are shown in WeaponListView
    - Runtime selected station display is shown in StationListView
    - MissionLoad remains the runtime source of truth
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionWeaponDlg.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UObject;
class UUserWidget;
class UWidget;

class UMissionBriefingDlg;
class UMissionLoadoutListView;
class UMissionPlanner;
class UMissionWeaponLoadoutListObject;
class UMissionWeaponStationRowObject;

class Mission;
class MissionElement;
class MissionLoad;

struct FShipDesign;
struct FShipLoadout;
struct FWeaponDesign;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponDlg : public UUserWidget
{
    GENERATED_BODY()

public:
    UMissionWeaponDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    void SetParentDlg(UMissionBriefingDlg* InParentDlg);
    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }

    void RefreshFromMission();

    void HandleStationChanged(int32 StationIndex, int32 NewSelection);

protected:
    // ------------------------------------------------------------
    // Mission / player resolution
    // ------------------------------------------------------------

    Mission* ResolveMission() const;
    MissionElement* ResolvePlayerElement() const;
    const FShipDesign* ResolvePlayerShipDesign() const;

    
    UWidget* BuildListHeaderRow(
        const FString& LeftText,
        const FString& RightText,
        float LeftWidth,
        float RightWidth) const;
    // ------------------------------------------------------------
    // UI build / refresh
    // ------------------------------------------------------------

    void BuildRuntimeLayout();
    void ClearLoadouts();
    void BuildLoadouts(MissionElement* Element, const FShipDesign* Design);
    void RefreshWeaponList();
    void RefreshSelectedLoadoutStations();

    // ------------------------------------------------------------
    // Selection helpers
    // ------------------------------------------------------------

    bool GetSelectedLoadoutName(MissionElement* Element, FString& OutName) const;
    void HandleLoadoutSelectionChanged(UObject* Item);
    void OnLoadoutSelected(UMissionWeaponLoadoutListObject* SelectedItem);

    // ------------------------------------------------------------
    // Display helpers
    // ------------------------------------------------------------

    FString GetElementName(MissionElement* E) const;
    FString GetDesignName(const FShipDesign* D) const;
    FString FormatWeight(double Mass) const;

    UTextBlock* BuildLabelText(const FString& Text) const;
    UTextBlock* BuildValueText(const FString& Text) const;
    UBorder* BuildHeader(const FString& Text) const;

    // ------------------------------------------------------------
    // Weapon / mass helpers
    // ------------------------------------------------------------

    const FWeaponDesign* ResolveWeaponDesignForStationSelection(
        const FShipDesign* Design,
        int32 StationIndex,
        int32 PointIndex) const;

    double ComputeCurrentCustomMass(
        MissionElement* Element,
        const FShipDesign* Design) const;

    double ComputeLoadoutMass(
        const FShipDesign* Design,
        const FShipLoadout& Loadout) const;

protected:
    // ------------------------------------------------------------
    // Root runtime host
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RuntimeHost = nullptr;

    // ------------------------------------------------------------
    // Parent / manager
    // ------------------------------------------------------------

    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

    UMissionPlanner* MissionScreen = nullptr;

    // ------------------------------------------------------------
    // List views
    // ------------------------------------------------------------

    UPROPERTY()
    UMissionLoadoutListView* WeaponListView = nullptr;

    UPROPERTY()
    UMissionLoadoutListView* StationListView = nullptr;

    // ------------------------------------------------------------
    // Row item storage
    // ------------------------------------------------------------

    UPROPERTY()
    TArray<TObjectPtr<UMissionWeaponLoadoutListObject>> Items;

    UPROPERTY()
    TArray<TObjectPtr<UMissionWeaponStationRowObject>> StationItems;

    // ------------------------------------------------------------
    // Header / info fields
    // ------------------------------------------------------------

    UPROPERTY()
    UTextBlock* ElementNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* DesignNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* WeightValueText = nullptr;

    // ------------------------------------------------------------
    // Entry widget classes
    // ------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Mission Weapon")
    TSubclassOf<UUserWidget> EntryWidgetClass;

    UPROPERTY(EditAnywhere, Category = "Mission Weapon")
    TSubclassOf<UUserWidget> StationEntryWidgetClass;

    // ------------------------------------------------------------
    // Refresh guard
    // ------------------------------------------------------------

    bool bRefreshingLoadouts = false;
};