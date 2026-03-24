/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionDebriefDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Mission Debriefing dialog (Unreal)
    - UMG/UBaseScreen version of DebriefDlg.frm
    - Displays mission header info, objectives/situation, score,
      unit list, per-unit summary stats, and mission event log.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "MissionDebriefDlg.generated.h"

// Forward declarations (legacy/ported sim classes):
class Campaign;
class Mission;
class MissionInfo;
class Sim;
class Ship;
class StarSystem;
class MissionElement;
class PlayerCharacter;
class ShipStats;
class SimEvent;

class UMissionPlanner;

UCLASS()
class STARSHATTERWARS_API UDebriefListItem : public UObject
{
    GENERATED_BODY()

public:
    // Generic 4-column row (UnitList uses 1..3, Summary/Event mostly use 0..2)
    UPROPERTY() FString Col0;
    UPROPERTY() FString Col1;
    UPROPERTY() FString Col2;
    UPROPERTY() FString Col3;

    // Optional payload (e.g., ShipStats index):
    UPROPERTY() int32 DataIndex = INDEX_NONE;

    static UDebriefListItem* Make(UObject* Outer,
        const FString& In0,
        const FString& In1 = TEXT(""),
        const FString& In2 = TEXT(""),
        const FString& In3 = TEXT(""),
        int32 InDataIndex = INDEX_NONE)
    {
        UDebriefListItem* Item = NewObject<UDebriefListItem>(Outer);
        Item->Col0 = In0;
        Item->Col1 = In1;
        Item->Col2 = In2;
        Item->Col3 = In3;
        Item->DataIndex = InDataIndex;
        return Item;
    }
};

UCLASS()
class STARSHATTERWARS_API UMissionDebriefDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionDebriefDlg(const FObjectInitializer& ObjectInitializer);

    // UBaseScreen (FORM binding hook):
    virtual void BindFormWidgets() override;

    // Dialog input hooks (Enter/Escape) via UBaseScreen:
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    // Show/Hide (Unreal-UMG style):
    void Show();
    void Hide();

    // Manager (raw pointer by request):
    void SetManager(UMissionPlanner* InManager) { Manager = InManager; }
    UMissionPlanner* GetManager() const { return Manager; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    // ------------------------------------------------------------
    // Event handlers
    // ------------------------------------------------------------
    UFUNCTION() void OnCloseClicked();
    UFUNCTION() void OnUnitSelectionChanged(UObject* SelectedItem);

    void ExecFrame(float DeltaSeconds);
    void DrawUnits();
    void DrawSelectedUnit(int32 StatsIndex);

private:
    // ------------------------------------------------------------
    // Owner
    // ------------------------------------------------------------
    UMissionPlanner* Manager = nullptr;

    // ------------------------------------------------------------
    // UMG Widgets (BindWidgetOptional)
    // These are OPTIONAL so the class compiles even if your widget
    // blueprint doesn't have them yet. BindFormWidgets() maps IDs.
    // ------------------------------------------------------------

    // Button:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CloseButtonWidget = nullptr;

    // Header fields:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionNameWidget = nullptr;       // id 200
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionSystemWidget = nullptr;     // id 202
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionSectorWidget = nullptr;     // id 204
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionTimeStartWidget = nullptr;  // id 206

    // Body text:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<URichTextBlock> ObjectivesWidget = nullptr;    // id 210
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<URichTextBlock> SituationWidget = nullptr;     // id 240
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MissionScoreWidget = nullptr;      // id 211

    // Lists:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> UnitListWidget = nullptr;           // id 320
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> SummaryListWidget = nullptr;        // id 330
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UListView> EventListWidget = nullptr;          // id 340

    // ------------------------------------------------------------
    // Legacy state (raw pointers, non-UObject)
    // ------------------------------------------------------------
    Campaign* CampaignPtr = nullptr;
    Mission* MissionPtr = nullptr;
    Sim* SimPtr = nullptr;
    Ship* PlayerShip = nullptr;

    bool bIsShown = false;
};
