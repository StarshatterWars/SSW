/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionEventDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEventDlg
    - Unreal UUserWidget replacement for legacy MsnEventDlg.
    - Uses standard UMG widget bindings (BindWidgetOptional).
    - Bindings correspond to MsnEventDlg.frm control IDs.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MenuScreen.h"
#include "MissionEventDlg.generated.h"

class UButton;
class UComboBoxString;
class UEditableTextBox;
class UTextBlock;

class Mission;
class MissionEvent;

UCLASS()
class STARSHATTERWARS_API UMissionEventDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionEventDlg(const FObjectInitializer& ObjectInitializer);
    void InitializeDlg(UMenuScreen* InManager);
    void SetMission(Mission* InMission);
    void SetMissionEvent(MissionEvent* InEvent);
    void ShowDlg();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void RebuildFromModel();
    void FillShipList(UComboBoxString* Combo, const char* SelectedAnsi);
    void FillRgnList(UComboBoxString* Combo, const char* SelectedAnsi);
    void RefreshTargetListForSelectedEvent();

    static bool ParseFloatBox(UEditableTextBox* Box, float& OutValue);
    static bool ParseIntBox(UEditableTextBox* Box, int32& OutValue);

private:
    UFUNCTION() void OnAcceptClicked();
    UFUNCTION() void OnCancelClicked();
    UFUNCTION() void OnEventSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

private:
    UPROPERTY(Transient)
    UMenuScreen* Manager = nullptr;

    Mission* MissionPtr = nullptr;
    MissionEvent* EventPtr = nullptr;

    // -----------------------------
    // Widget bindings (OPTIONAL)
    // -----------------------------

    // id 201 (legacy label). Use TextBlock in UMG.
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IdLabel = nullptr;              // id 201

    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TimeEdit = nullptr;       // id 202
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* DelayEdit = nullptr;      // id 203

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* EventCombo = nullptr;      // id 204
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* EventShipCombo = nullptr;  // id 205
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* EventSourceCombo = nullptr;// id 206
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* EventTargetCombo = nullptr;// id 207

    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* EventParamEdit = nullptr; // id 208
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* EventSoundEdit = nullptr; // id 209
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* EventMessageEdit = nullptr;// id 210

    // right column:
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* ChanceEdit = nullptr;     // id 220

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* TriggerCombo = nullptr;    // id 221
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* TriggerShipCombo = nullptr;// id 222
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* TriggerTargetCombo = nullptr;// id 223
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TriggerParamEdit = nullptr;// id 224

    // OK/Cancel
    UPROPERTY(meta = (BindWidgetOptional)) UButton* AcceptButton = nullptr; // 1
};
