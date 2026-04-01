/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission weapon dialog.

    Displays available preset loadouts for the player ship and
    verifies runtime MissionLoad station data prior to custom
    per-station editing.

    NOTES
    =====
    - Current visible UI still shows preset loadouts
    - Runtime MissionLoad path is verified via logs
    - Guard flag prevents recursive selection rebuilds
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MissionWeaponDlg.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;
class UObject;
class UMissionBriefingDlg;
class UMissionLoadoutListView;
class UMissionWeaponLoadoutListObject;
class UWidget;
class UMissionPlanner;

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

protected:
    Mission* ResolveMission() const;
    MissionElement* ResolvePlayerElement() const;
    const FShipDesign* ResolvePlayerShipDesign() const;

    void BuildRuntimeLayout();
    void ClearLoadouts();
    void BuildLoadouts(MissionElement* Element, const FShipDesign* Design);
    void RefreshWeaponList();

    bool GetSelectedLoadoutName(MissionElement* Element, FString& OutName) const;

    FString GetElementName(MissionElement* E) const;
    FString GetDesignName(const FShipDesign* D) const;
    FString FormatWeight(double Mass) const;

    UTextBlock* BuildLabelText(const FString& Text) const;
    UTextBlock* BuildValueText(const FString& Text) const;
    UBorder* BuildHeader(const FString& Text) const;

    void HandleLoadoutSelectionChanged(UObject* Item);
    void OnLoadoutSelected(UMissionWeaponLoadoutListObject* SelectedItem);

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
    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RuntimeHost = nullptr;

    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY()
    UMissionLoadoutListView* WeaponListView = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UMissionWeaponLoadoutListObject>> Items;

    UPROPERTY()
    UTextBlock* ElementNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* DesignNameValueText = nullptr;

    UPROPERTY()
    UTextBlock* WeightValueText = nullptr;

    UPROPERTY(EditAnywhere, Category = "Mission Weapon")
    TSubclassOf<UUserWidget> EntryWidgetClass;

    bool bRefreshingLoadouts = false;

private:
    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;
    UMissionPlanner* MissionScreen = nullptr;
};