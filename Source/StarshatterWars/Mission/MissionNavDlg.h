/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionNavDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionNavDlg

    Navigation panel hosted by UMissionBriefingDlg.

    This widget owns NAV-local UI only:

      - Top NAV mode buttons (GALAXY / SYSTEM / SECTOR)
      - Zoom buttons (- / +)
      - Local NAV mode switcher
      - Right-side radio filter buttons
      - Object list panel
      - Detail panel

    The parent UMissionBriefingDlg owns:

      - Main briefing header
      - SIT / PKG / NAV / WEP switching
      - Accept / Cancel flow
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionNavDlg.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UListView;
class UMenuButton;
class USizeBox;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class UWidgetSwitcher;

class UMissionBriefingDlg;
class UMissionPlanner;

class Campaign;
class Mission;
class MissionInfo;

UENUM()
enum class EMissionNavMode : uint8
{
    GALAXY = 0,
    SYSTEM,
    SECTOR
};

UENUM()
enum class EMissionNavFilterMode : uint8
{
    SYSTEM = 0,
    PLANET,
    SECTOR,
    STATION,
    STARSHIP,
    FIGHTER
};

UCLASS()
class STARSHATTERWARS_API UMissionNavDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionNavDlg(const FObjectInitializer& ObjectInitializer);

    // -----------------------------------------------------------------
    // Initialization / Context
    // -----------------------------------------------------------------

    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    void SetParentDlg(UMissionBriefingDlg* InParentDlg);

    void RefreshFromMission();

protected:
    // -----------------------------------------------------------------
    // UE Overrides
    // -----------------------------------------------------------------

    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // -----------------------------------------------------------------
    // Internal Helpers
    // -----------------------------------------------------------------

    Mission* ResolveMission() const;

    void BuildRuntimeLayout();
    void BuildNavModeButtons();
    void BuildRightPanels();
    void BuildFilterButtons();

    void RefreshNavModeSelection();
    void RefreshFilterSelection();
    void RefreshObjectListPanel();
    void RefreshDetailPanel();

    void SetNavMode(EMissionNavMode NewMode);
    void SetFilterMode(EMissionNavFilterMode NewMode);

    FString GetFilterModeLabel(EMissionNavFilterMode Mode) const;

    UMenuButton* CreateNavModeButton(const FString& Label, UHorizontalBox* ParentBox);
    UMenuButton* CreateFilterButton(const FString& Label, int32 Row, int32 Column);

protected:
    // -----------------------------------------------------------------
    // Parent / Manager
    // -----------------------------------------------------------------

    UPROPERTY()
    UMissionBriefingDlg* ParentDlg = nullptr;

    UPROPERTY(Transient)
    UMissionPlanner* Manager = nullptr;

private:
    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    MissionInfo* MissionInfoPtr = nullptr;

    // -----------------------------------------------------------------
    // Runtime Host
    // -----------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RuntimeHost = nullptr;

    // -----------------------------------------------------------------
    // Runtime Layout
    // -----------------------------------------------------------------

    UPROPERTY()
    UVerticalBox* MainColumn = nullptr;

    UPROPERTY()
    UHorizontalBox* TopButtonRow = nullptr;

    UPROPERTY()
    UHorizontalBox* NavModeButtonBox = nullptr;

    UPROPERTY()
    UHorizontalBox* ZoomButtonBox = nullptr;

    UPROPERTY()
    UHorizontalBox* ContentRow = nullptr;

    UPROPERTY()
    USizeBox* MainViewHost = nullptr;

    UPROPERTY()
    UVerticalBox* RightPanelColumn = nullptr;

    // -----------------------------------------------------------------
    // Top Buttons
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "Mission Nav")
    TSubclassOf<UMenuButton> MenuButtonClass;

    UPROPERTY()
    TArray<TObjectPtr<UMenuButton>> NavModeButtons;

    UPROPERTY()
    UButton* ZoomOutButton = nullptr;

    UPROPERTY()
    UButton* ZoomInButton = nullptr;

    UPROPERTY()
    UTextBlock* ZoomOutText = nullptr;

    UPROPERTY()
    UTextBlock* ZoomInText = nullptr;

    // -----------------------------------------------------------------
    // Local NAV Switcher
    // -----------------------------------------------------------------

    UPROPERTY()
    UWidgetSwitcher* NavSwitcher = nullptr;

    UPROPERTY()
    USizeBox* GalaxyPanelHost = nullptr;

    UPROPERTY()
    USizeBox* SystemPanelHost = nullptr;

    UPROPERTY()
    USizeBox* SectorPanelHost = nullptr;

    UPROPERTY()
    UTextBlock* NavBodyText = nullptr;

    // -----------------------------------------------------------------
    // Right Panel - Filter Selection
    // -----------------------------------------------------------------

    UPROPERTY()
    UVerticalBox* FilterPanelHost = nullptr;

    UPROPERTY()
    UUniformGridPanel* FilterButtonGrid = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UMenuButton>> FilterButtons;

    // -----------------------------------------------------------------
    // Right Panel - Object List
    // -----------------------------------------------------------------

    UPROPERTY()
    UBorder* ObjectListBorder = nullptr;

    UPROPERTY()
    UVerticalBox* ObjectListPanel = nullptr;

    UPROPERTY()
    UTextBlock* ObjectListTitleText = nullptr;

    UPROPERTY()
    USizeBox* ObjectListHost = nullptr;

    UPROPERTY()
    UListView* ObjectListView = nullptr;

    // -----------------------------------------------------------------
    // Right Panel - Detail Panel
    // -----------------------------------------------------------------

    UPROPERTY()
    UBorder* DetailBorder = nullptr;

    UPROPERTY()
    UVerticalBox* DetailPanel = nullptr;

    UPROPERTY()
    UTextBlock* DetailTitleText = nullptr;

    UPROPERTY()
    USizeBox* DetailHost = nullptr;

    UPROPERTY()
    UTextBlock* DetailBodyText = nullptr;

    // -----------------------------------------------------------------
    // State
    // -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionNav|State", meta = (AllowPrivateAccess = "true"))
    EMissionNavMode CurrentNavMode = EMissionNavMode::SYSTEM;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionNav|State", meta = (AllowPrivateAccess = "true"))
    EMissionNavFilterMode CurrentFilterMode = EMissionNavFilterMode::SYSTEM;

private:
    // -----------------------------------------------------------------
    // Event Handlers
    // -----------------------------------------------------------------

    UFUNCTION()
    void OnNavModeButtonSelected(UMenuButton* SelectedButton);

    UFUNCTION()
    void OnNavModeButtonHovered(UMenuButton* HoveredButton);

    UFUNCTION()
    void OnFilterButtonSelected(UMenuButton* SelectedButton);

    UFUNCTION()
    void OnFilterButtonHovered(UMenuButton* HoveredButton);

    UFUNCTION()
    void OnZoomInClicked();

    UFUNCTION()
    void OnZoomOutClicked();
};