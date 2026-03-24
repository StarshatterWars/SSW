/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         EngineeringDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Renamed from UEngDlg to UEngineeringDlg to avoid UHT engine-name collisions.
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves original member names and intent where applicable.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "GameScreen.h"

#include "List.h"
#include "ThreadSync.h"
#include "EngineeringDlg.generated.h"



// UMG fwd:
class UButton;
class UListView;
class UTextBlock;
class URichTextBlock;
class USlider;

// Starshatter fwd:
class Ship;
class PowerSource;
class SimSystem;
class SimComponent;

// Legacy containers/types (ported in your codebase):
template<typename T> class List;

UCLASS()
class STARSHATTERWARS_API UEngineeringDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UEngineeringDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // UUserWidget lifecycle
    // ----------------------------------------------------------------
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ----------------------------------------------------------------
    // Legacy dialog surface (ported)
    // ----------------------------------------------------------------
    virtual void      Show();
    virtual void      Hide();
    virtual void      RegisterControls();

    // Operations (AWEvent -> UFUNCTION handlers):
    UFUNCTION() virtual void OnSource(int SourceIndex);
    UFUNCTION() virtual void OnClient(int SourceIndex);
    UFUNCTION() virtual void OnRouteStart(int SourceIndex);
    UFUNCTION() virtual void OnRouteComplete(int DestIndex);

    UFUNCTION() virtual void OnPowerOff();
    UFUNCTION() virtual void OnPowerOn();
    UFUNCTION() virtual void OnOverride();
    UFUNCTION() virtual void OnPowerLevelChanged(float Value);

    UFUNCTION() virtual void OnComponentSelected();
    UFUNCTION() virtual void OnAutoRepair();
    UFUNCTION() virtual void OnRepair();
    UFUNCTION() virtual void OnReplace();
    UFUNCTION() virtual void OnQueueSelected();
    UFUNCTION() virtual void OnPriorityIncrease();
    UFUNCTION() virtual void OnPriorityDecrease();
    UFUNCTION() virtual void OnClose();

    void              ExecFrame(float DeltaTime);
    void              UpdateRouteTables();
    void              UpdateSelection();
    void              SetShip(Ship* s);

protected:
    // ----------------------------------------------------------------
    // UBaseScreen overrides
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

public:
    // Owner (set by game screen):
    UGameScreen* manager = nullptr;

protected:
    // ----------------------------------------------------------------
    // Bound UMG controls (match FORM ids)
    //
    // NOTE:
    // The classic dialog has 4 columns of "sources + slider + list".
    // In UMG, you typically implement each as:
    // - UButton* source button
    // - USlider* source slider
    // - UListView* client list
    //
    // You can either bind these as individual widgets if you created them
    // in the UMG designer, or gather them dynamically in BindFormWidgets().
    // ----------------------------------------------------------------

    // Close button (FORM id 1):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* close_btn = nullptr;

    // Source buttons (FORM ids 201-204):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* source_btn_0 = nullptr; // 201
    UPROPERTY(meta = (BindWidgetOptional)) UButton* source_btn_1 = nullptr; // 202
    UPROPERTY(meta = (BindWidgetOptional)) UButton* source_btn_2 = nullptr; // 203
    UPROPERTY(meta = (BindWidgetOptional)) UButton* source_btn_3 = nullptr; // 204

    // Source sliders (FORM ids 211-214):
    UPROPERTY(meta = (BindWidgetOptional)) USlider* source_lvl_0 = nullptr; // 211
    UPROPERTY(meta = (BindWidgetOptional)) USlider* source_lvl_1 = nullptr; // 212
    UPROPERTY(meta = (BindWidgetOptional)) USlider* source_lvl_2 = nullptr; // 213
    UPROPERTY(meta = (BindWidgetOptional)) USlider* source_lvl_3 = nullptr; // 214

    // Client lists (FORM ids 301-304):
    UPROPERTY(meta = (BindWidgetOptional)) UListView* clients_0 = nullptr; // 301
    UPROPERTY(meta = (BindWidgetOptional)) UListView* clients_1 = nullptr; // 302
    UPROPERTY(meta = (BindWidgetOptional)) UListView* clients_2 = nullptr; // 303
    UPROPERTY(meta = (BindWidgetOptional)) UListView* clients_3 = nullptr; // 304

    // Selected name label (FORM id 401):
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* selected_name = nullptr;

    // Power toggles (FORM ids 402-403):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* power_off = nullptr; // 402
    UPROPERTY(meta = (BindWidgetOptional)) UButton* power_on = nullptr;  // 403

    // Override (FORM id 410):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* override_btn = nullptr; // 410

    // Power allocation slider (FORM id 404):
    UPROPERTY(meta = (BindWidgetOptional)) USlider* power_level = nullptr; // 404

    // Capacity slider (FORM id 405) - read-only in classic:
    UPROPERTY(meta = (BindWidgetOptional)) USlider* capacity = nullptr; // 405

    // Auto repair (FORM id 700):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* auto_repair = nullptr; // 700

    // Components list (FORM id 501):
    UPROPERTY(meta = (BindWidgetOptional)) UListView* components = nullptr; // 501

    // Repair / Replace (FORM ids 502-503):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* repair = nullptr;  // 502
    UPROPERTY(meta = (BindWidgetOptional)) UButton* replace = nullptr; // 503

    // Repair/Replace times (FORM ids 512-513):
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* repair_time = nullptr;  // 512
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* replace_time = nullptr; // 513

    // Repair queue list (FORM id 601):
    UPROPERTY(meta = (BindWidgetOptional)) UListView* repair_queue = nullptr; // 601

    // Priority buttons (FORM ids 602-603):
    UPROPERTY(meta = (BindWidgetOptional)) UButton* priority_increase = nullptr; // 602
    UPROPERTY(meta = (BindWidgetOptional)) UButton* priority_decrease = nullptr; // 603

protected:
    // ----------------------------------------------------------------
    // Legacy state (ported)
    // ----------------------------------------------------------------
    Ship* ship = nullptr;
    PowerSource* route_source = nullptr;

    // route_list: clients being dragged from one reactor to another
    List<SimSystem> route_list;

    PowerSource* selected_source = nullptr;
    List<SimSystem> selected_clients;

    SimSystem* selected_repair = nullptr;
    SimComponent* selected_component = nullptr;

    // Threading placeholder (kept for parity with other screens):
    ThreadSync   sync;

private:
    // Helpers: gather 4-way arrays from optional bound widgets:
    void GatherSourceWidgets();
    void BindWidgetClicks();

    // Cached widget arrays (not UPROPERTY; derived from the bound pointers):
    UButton* SourceButtons[4] = { nullptr, nullptr, nullptr, nullptr };
    USlider* SourceLevels[4] = { nullptr, nullptr, nullptr, nullptr };
    UListView* ClientLists[4] = { nullptr, nullptr, nullptr, nullptr };
};
