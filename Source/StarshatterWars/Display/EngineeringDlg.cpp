/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         EngineeringDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Renamed from UEngDlg to UEngineeringDlg to avoid UHT engine-name collisions.
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves original member names and intent where applicable.

    NOTES:
    - This is a functional skeleton that mirrors the classic structure.
    - ListBox drag/drop and per-row coloring require UListView row objects + entry widgets.
*/

#include "EngineeringDlg.h"

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"

#include "Game.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "FormatUtil.h"

// Starshatter core (ported):
#include "GameScreen.h"
#include "Ship.h"
#include "Power.h"
#include "SimComponent.h"
#include "SimSystem.h"

// +--------------------------------------------------------------------+

UEngineeringDlg::UEngineeringDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UEngineeringDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BindFormWidgets();
    RegisterControls();
}

void UEngineeringDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

void UEngineeringDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

// --------------------------------------------------------------------
// UBaseScreen overrides
// --------------------------------------------------------------------

void UEngineeringDlg::BindFormWidgets()
{
    // If your UBaseScreen already auto-binds by id/name, keep this empty.
    // Otherwise, you can gather widgets here (GetWidgetFromName, etc.).
    GatherSourceWidgets();
}

FString UEngineeringDlg::GetLegacyFormText() const
{
    // If you are using your legacy .frm loader, return the EngDlg.frm here.
    // Otherwise return empty.
    return TEXT("");
}

// --------------------------------------------------------------------
// Setup
// --------------------------------------------------------------------

void UEngineeringDlg::GatherSourceWidgets()
{
    SourceButtons[0] = source_btn_0;
    SourceButtons[1] = source_btn_1;
    SourceButtons[2] = source_btn_2;
    SourceButtons[3] = source_btn_3;

    SourceLevels[0] = source_lvl_0;
    SourceLevels[1] = source_lvl_1;
    SourceLevels[2] = source_lvl_2;
    SourceLevels[3] = source_lvl_3;

    ClientLists[0] = clients_0;
    ClientLists[1] = clients_1;
    ClientLists[2] = clients_2;
    ClientLists[3] = clients_3;
}

void UEngineeringDlg::BindWidgetClicks()
{
    if (close_btn) close_btn->OnClicked.AddDynamic(this, &UEngineeringDlg::OnClose);

    if (power_off) power_off->OnClicked.AddDynamic(this, &UEngineeringDlg::OnPowerOff);
    if (power_on)  power_on->OnClicked.AddDynamic(this, &UEngineeringDlg::OnPowerOn);
    if (override_btn) override_btn->OnClicked.AddDynamic(this, &UEngineeringDlg::OnOverride);

    if (auto_repair) auto_repair->OnClicked.AddDynamic(this, &UEngineeringDlg::OnAutoRepair);
    if (repair)      repair->OnClicked.AddDynamic(this, &UEngineeringDlg::OnRepair);
    if (replace)     replace->OnClicked.AddDynamic(this, &UEngineeringDlg::OnReplace);

    if (priority_increase) priority_increase->OnClicked.AddDynamic(this, &UEngineeringDlg::OnPriorityIncrease);
    if (priority_decrease) priority_decrease->OnClicked.AddDynamic(this, &UEngineeringDlg::OnPriorityDecrease);

    if (power_level) {
        power_level->OnValueChanged.AddDynamic(this, &UEngineeringDlg::OnPowerLevelChanged);
    }

    // Source buttons: we need per-index binding. UMG doesn't support binding lambdas
    // with AddDynamic, so you typically create 4 wrapper UFUNCTIONs.
    // This implementation keeps a clean API and expects you to call OnSource(Index)
    // from wrapper functions or entry widgets.
}

void UEngineeringDlg::RegisterControls()
{
    GatherSourceWidgets();
    BindWidgetClicks();

    // Disable until we have a ship:
    if (repair)  repair->SetIsEnabled(false);
    if (replace) replace->SetIsEnabled(false);

    if (priority_increase) priority_increase->SetIsEnabled(false);
    if (priority_decrease) priority_decrease->SetIsEnabled(false);

    if (repair_time)   repair_time->SetVisibility(ESlateVisibility::Collapsed);
    if (replace_time)  replace_time->SetVisibility(ESlateVisibility::Collapsed);
}

// --------------------------------------------------------------------
// Show/Hide
// --------------------------------------------------------------------

void UEngineeringDlg::Show()
{
    // In UE: AddToViewport/SetVisibility handled by owning screen stack.
    // Keep refresh behavior:
    UpdateRouteTables();
}

void UEngineeringDlg::Hide()
{
    // In UE: SetVisibility/RemoveFromParent handled by owner.
}

// --------------------------------------------------------------------
// Runtime
// --------------------------------------------------------------------

void UEngineeringDlg::ExecFrame(float DeltaTime)
{
    // Classic behavior: only update selection if shown.
    // If you have a screen-stack boolean, check it. Otherwise just update if ship exists.
    if (ship) {
        UpdateSelection();
    }
}

void UEngineeringDlg::SetShip(Ship* s)
{
    if (ship != s) {
        selected_source = nullptr;
        selected_repair = nullptr;
        selected_component = nullptr;
        selected_clients.clear();

        ship = s;

        if (ship) {
            UpdateRouteTables();
        }
    }
    else if (!s) {
        ship = nullptr;
    }
}

// --------------------------------------------------------------------
// Table population (placeholder for UListView row items)
// --------------------------------------------------------------------

void UEngineeringDlg::UpdateRouteTables()
{
    // This should rebuild the four client lists, sliders, and button texts.
    // In classic: reactors[] drives visibility for up to 4.
    // In UE: you must create UObject list items per row and set them into UListView.

    if (!ship)
        return;

    const int32 NumSources = ship->Reactors().size();

    for (int32 i = 0; i < 4; i++) {
        if (SourceButtons[i]) SourceButtons[i]->SetVisibility(i < NumSources ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (SourceLevels[i])  SourceLevels[i]->SetVisibility(i < NumSources ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (ClientLists[i])   ClientLists[i]->SetVisibility(i < NumSources ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

        if (ClientLists[i]) {
            // Clear list items:
            ClientLists[i]->ClearListItems();
        }
    }

    // TODO:
    // - Create a UObject item per System client, including display name + power level + status color.
    // - Add items to ClientLists[source_index].
    // - Drive SourceLevels[i] value from reactor power level.
}

// --------------------------------------------------------------------
// Selection updates (mirrors classic logic at high-level)
// --------------------------------------------------------------------

void UEngineeringDlg::UpdateSelection()
{
    if (!ship)
        return;

    // This is where the classic UI continuously updates:
    // - client power numbers
    // - colors per status
    // - selected detail panel (selected_name, power toggles, override, sliders)
    //
    // In UMG, the per-row updates belong in the row object/widget.
    // Here we keep only the detail-panel logic.

    if (selected_source) {
        if (selected_name)
            selected_name->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText(selected_source->Name()))));

        if (power_off) power_off->SetIsEnabled(true);
        if (power_on)  power_on->SetIsEnabled(true);
        if (override_btn) override_btn->SetIsEnabled(true);

        if (power_level) {
            power_level->SetIsEnabled(selected_source->IsPowerOn());
            power_level->SetValue((float)selected_source->GetPowerLevel() / 100.0f);
        }

        if (capacity) {
            capacity->SetIsEnabled(true);
            capacity->SetValue((float)selected_source->Charge() / 100.0f);
        }
    }
    else if (selected_clients.size() > 0) {
        SimSystem* Sink = selected_clients[0];
        if (selected_name)
            selected_name->SetText(FText::FromString(UTF8_TO_TCHAR(Game::GetText(Sink->Name()))));

        if (power_off) power_off->SetIsEnabled(true);
        if (power_on)  power_on->SetIsEnabled(true);
        if (override_btn) override_btn->SetIsEnabled(true);

        if (power_level) {
            power_level->SetIsEnabled(Sink->IsPowerOn());
            power_level->SetValue((float)Sink->GetPowerLevel() / 100.0f);
        }

        if (capacity) {
            capacity->SetIsEnabled(true);
            capacity->SetValue((float)Sink->Charge() / 100.0f);
        }
    }
    else {
        if (selected_name)
            selected_name->SetText(FText::FromString(TEXT("No Selection")));

        if (power_off) power_off->SetIsEnabled(false);
        if (power_on)  power_on->SetIsEnabled(false);
        if (override_btn) override_btn->SetIsEnabled(false);

        if (power_level) {
            power_level->SetIsEnabled(false);
            power_level->SetValue(0.0f);
        }

        if (capacity) {
            capacity->SetIsEnabled(false);
            capacity->SetValue(0.0f);
        }
    }
}

// --------------------------------------------------------------------
// UI actions (ported intent)
// --------------------------------------------------------------------

void UEngineeringDlg::OnSource(int SourceIndex)
{
    selected_source = nullptr;
    selected_clients.clear();
    selected_component = nullptr;

    if (!ship)
        return;

    if (SourceIndex >= 0 && SourceIndex < ship->Reactors().size()) {
        selected_source = ship->Reactors()[SourceIndex];
    }

    // Clear client selections across all lists:
    for (int i = 0; i < 4; i++) {
        if (ClientLists[i]) {
            ClientLists[i]->ClearSelection();
        }
    }

    // Components list should be rebuilt based on selected_source:
    if (components) {
        components->ClearListItems();
        if (repair)  repair->SetIsEnabled(false);
        if (replace) replace->SetIsEnabled(false);
        if (repair_time)  repair_time->SetVisibility(ESlateVisibility::Collapsed);
        if (replace_time) replace_time->SetVisibility(ESlateVisibility::Collapsed);

        // TODO: create component row objects for selected_source->GetComponents()
    }
}

void UEngineeringDlg::OnClient(int SourceIndex)
{
    selected_source = nullptr;
    selected_clients.clear();
    selected_component = nullptr;

    if (!ship)
        return;

    // In UE, selection retrieval comes from the UListView's selected objects.
    // This method is kept as the ported "intent" hook; you will call it from
    // your row selection handler and build selected_clients accordingly.
}

void UEngineeringDlg::OnRouteStart(int SourceIndex)
{
    if (!ship)
        return;

    // In classic: builds route_list from selected items in clients[sourceIndex].
    // In UE: drag start occurs in the row widget; gather selected row objects there,
    // then call into this function or set route_list directly.
}

void UEngineeringDlg::OnRouteComplete(int DestIndex)
{
    if (!ship || !route_source)
        return;

    if (DestIndex < 0 || DestIndex >= ship->Reactors().size())
        return;

    PowerSource* RouteDest = ship->Reactors()[DestIndex];
    if (!RouteDest)
        return;

    // Classic logic:
    // for each System in route_list: route_source->RemoveClient(client); RouteDest->AddClient(client);
    ListIter<SimSystem> it = route_list;
    while (++it) {
        SimSystem* Client = it.value();
        route_source->RemoveClient(Client);
        RouteDest->AddClient(Client);
    }

    UpdateRouteTables();
}

void UEngineeringDlg::OnPowerOff()
{
    if (selected_source) {
        selected_source->PowerOff();
    }
    else if (selected_clients.size() > 0) {
        ListIter<SimSystem> it = selected_clients;
        while (++it)
            it->PowerOff();
    }
}

void UEngineeringDlg::OnPowerOn()
{
    if (selected_source) {
        selected_source->PowerOn();
    }
    else if (selected_clients.size() > 0) {
        ListIter<SimSystem> it = selected_clients;
        while (++it)
            it->PowerOn();
    }
}

void UEngineeringDlg::OnOverride()
{
    // In classic, override button state toggles.
    // In UE, you should track state with a bool or by reading a checkbox-like widget.
    const bool bOver = true; // TODO: replace with actual UI state

    if (selected_source) {
        selected_source->SetOverride(bOver);
    }
    else if (selected_clients.size() > 0) {
        ListIter<SimSystem> it = selected_clients;
        while (++it)
            it->SetOverride(bOver);
    }
}

void UEngineeringDlg::OnPowerLevelChanged(float Value)
{
    // Value is 0..1 in UMG slider by default.
    int Level = FMath::Clamp((int)FMath::RoundToInt(Value * 100.0f), 0, 100);

    if (selected_source) {
        selected_source->SetPowerLevel(Level);
    }
    else if (selected_clients.size() > 0) {
        ListIter<SimSystem> it = selected_clients;
        while (++it)
            it->SetPowerLevel(Level);
    }
}

void UEngineeringDlg::OnComponentSelected()
{
    // In UE, component selection comes from components->GetSelectedItem().
    // Keep hook; fill selected_component + enable repair/replace.
}

void UEngineeringDlg::OnAutoRepair()
{
    if (ship) {
        // In classic: ship->EnableRepair(auto_repair->GetButtonState() > 0);
        // In UE: track a bool toggle; placeholder:
        ship->EnableRepair(true);
    }
}

void UEngineeringDlg::OnRepair()
{
    if (selected_component) {
        selected_component->Repair();
        if (ship)
            ship->RepairSystem(selected_component->GetSystem());
    }
}

void UEngineeringDlg::OnReplace()
{
    if (selected_component) {
        selected_component->Replace();
        if (ship)
            ship->RepairSystem(selected_component->GetSystem());
    }
}

void UEngineeringDlg::OnQueueSelected()
{
    // In UE: selection comes from repair_queue->GetSelectedItem().
    // Keep hook; set selected_repair and enable priority buttons.
}

void UEngineeringDlg::OnPriorityIncrease()
{
    if (ship && repair_queue) {
        // In classic: ship->IncreaseRepairPriority(repair_queue->GetSelection());
        // In UE: you must map selection index.
    }
}

void UEngineeringDlg::OnPriorityDecrease()
{
    if (ship && repair_queue) {
        // In classic: ship->DecreaseRepairPriority(repair_queue->GetSelection());
        // In UE: you must map selection index.
    }
}

void UEngineeringDlg::OnClose()
{
    if (manager) {
        manager->CloseTopmost();
    }
}
