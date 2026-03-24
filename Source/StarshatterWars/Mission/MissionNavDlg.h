/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionNavDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEventDlg
    - Unreal UUserWidget replacement for legacy MsnEventDlg.
    - Uses standard UMG widget bindings (BindWidgetOptional).
    - Bindings correspond to MsnNavDlg.frm control IDs.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"                // <- your Unreal BaseScreen
#include "MissionNavDlg.generated.h"

class UButton;
class UTextBlock;
class UListView;

class UMissionPlanner;
class Campaign;
class Mission;
class MissionInfo;

UCLASS()
class STARSHATTERWARS_API UMissionNavDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionNavDlg(const FObjectInitializer& ObjectInitializer);

    void InitializeDlg(UMissionPlanner* InManager);
    void SetMissionContext(Campaign* InCampaign, Mission* InMission, MissionInfo* InInfo);
    void ShowDlg();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void RefreshHeader();
    void RefreshLists();

    UFUNCTION() void OnAcceptClicked();
    UFUNCTION() void OnCancelClicked();

    UFUNCTION() void OnTabSitClicked();
    UFUNCTION() void OnTabPkgClicked();
    UFUNCTION() void OnTabMapClicked();
    UFUNCTION() void OnTabWepClicked();

    UFUNCTION() void OnNavGalaxyClicked();
    UFUNCTION() void OnNavSystemClicked();
    UFUNCTION() void OnNavSectorClicked();
    UFUNCTION() void OnZoomInClicked();
    UFUNCTION() void OnZoomOutClicked();

    UFUNCTION() void OnFilterSystemClicked();
    UFUNCTION() void OnFilterPlanetClicked();
    UFUNCTION() void OnFilterSectorClicked();
    UFUNCTION() void OnFilterStationClicked();
    UFUNCTION() void OnFilterStarshipClicked();
    UFUNCTION() void OnFilterFighterClicked();

private:
    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    MissionInfo* MissionInfoPtr = nullptr;

    // ---- Tabs (frm ids 900-903) ----
    UPROPERTY(meta = (BindWidgetOptional)) UButton* TabSitButton = nullptr; // 900
    UPROPERTY(meta = (BindWidgetOptional)) UButton* TabPkgButton = nullptr; // 901
    UPROPERTY(meta = (BindWidgetOptional)) UButton* TabMapButton = nullptr; // 902
    UPROPERTY(meta = (BindWidgetOptional)) UButton* TabWepButton = nullptr; // 903

    // ---- Header (frm ids 200,202,204,206) ----
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* HeaderTitleText = nullptr; // 200
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* SystemValueText = nullptr; // 202
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* SectorValueText = nullptr; // 204
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* TimeText = nullptr;        // 206

    // ---- Briefing body (frm id 100) ----
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* BriefingBodyText = nullptr; // 100

    // ---- Nav buttons (frm ids 101-103) ----
    UPROPERTY(meta = (BindWidgetOptional)) UButton* GalaxyButton = nullptr; // 101
    UPROPERTY(meta = (BindWidgetOptional)) UButton* SystemButton = nullptr; // 102
    UPROPERTY(meta = (BindWidgetOptional)) UButton* SectorButton = nullptr; // 103

    // ---- Zoom (frm ids 110-111) ----
    UPROPERTY(meta = (BindWidgetOptional)) UButton* ZoomInButton = nullptr;  // 110
    UPROPERTY(meta = (BindWidgetOptional)) UButton* ZoomOutButton = nullptr; // 111

    // ---- Filters (frm ids 401-406) ----
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterSystemButton = nullptr;   // 401
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterPlanetButton = nullptr;   // 402
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterSectorButton = nullptr;   // 403
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterStationButton = nullptr;  // 404
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterStarshipButton = nullptr; // 405
    UPROPERTY(meta = (BindWidgetOptional)) UButton* FilterFighterButton = nullptr;  // 406

    // ---- Lists (frm ids 801-802) ----
    UPROPERTY(meta = (BindWidgetOptional)) UListView* ObjectList = nullptr; // 801
    UPROPERTY(meta = (BindWidgetOptional)) UListView* DetailList = nullptr; // 802

    // ---- Footer (frm ids 1-2) ----
    UPROPERTY(meta = (BindWidgetOptional)) UButton* AcceptButton = nullptr; // 1
};

