/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPackageDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Package Subpanel.

    Displays:
    - Friendly package list
    - Navigation plan
    - Threat summary

    DATA FLOW
    =========
    DataTable -> FShipDesign -> MissionElement -> UI
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionPackageDlg.generated.h"

class UListView;
class UTextBlock;
class USizeBox;
class UHorizontalBox;
class UWidget;
class UMissionPlanner;
class UMissionBriefingDlg;
class UMissionPackageListObject;
class UMissionNavListObject;
class Mission;
class MissionElement;

UCLASS()
class STARSHATTERWARS_API UMissionPackageDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionPackageDlg(const FObjectInitializer& ObjectInitializer);

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    void SetParentDlg(UMissionBriefingDlg* InParentDlg);

    void RefreshFromMission();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    Mission* ResolveMission() const;
    MissionElement* ResolveSelectedPackageElement() const;

    void DrawPackages();
    void DrawNavPlan();
    void DrawThreats();

    void BuildHeaders();
    void BuildPackageHeaderRow();
    void BuildNavHeaderRow();

    UWidget* MakeHeaderCell(const FString& Text, float Width) const;

    float GetPackageHeaderClampWidth() const;
    float GetNavHeaderClampWidth() const;

    UFUNCTION()
    void OnPackageSelectionChanged(UObject* Item);

private:
    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* PanelSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UHorizontalBox* PackageHeaderRow = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UHorizontalBox* NavHeaderRow = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* PackageTableSizebox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* PackageList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* NavList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MissionRosterLabel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Threat0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Threat1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Threat2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Threat3 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Threat4 = nullptr;

private:
    UPROPERTY()
    TArray<TObjectPtr<UMissionPackageListObject>> PackageItems;

    UPROPERTY()
    TArray<TObjectPtr<UMissionNavListObject>> NavItems;

    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

    int32 PackageIndex = INDEX_NONE;

    bool bHeadersBuiltFromGeometry = false;
};