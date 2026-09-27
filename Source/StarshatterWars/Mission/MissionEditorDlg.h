/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionEditorDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEditorDlg
    - Unreal UUserWidget replacement for legacy MsnEditDlg.
    - Inherits from UBaseScreen to use legacy FORM parsing.
    - Implements mission editor dialog behavior (tabs, fields, lists, actions).
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionEditorDlg.generated.h"

class UMenuScreen;
class Campaign;
class Mission;
class MissionInfo;

// Optional forward declarations for legacy dialogs you may already be porting:
class UMsnElemDlg;
class UMsnEventDlg;
class UNavDlg;

UCLASS()
class STARSHATTERWARS_API UMissionEditorDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionEditorDlg(const FObjectInitializer& ObjectInitializer);

    void InitializeMissionEditor(UMenuScreen* InManager);
    void SetMission(Mission* InMission);
    void SetMissionInfo(MissionInfo* InMissionInfo);
    void Show();
    void ShowTab(int32 Tab);

protected:
    // Legacy behaviors
    void ScrapeForm();
    void DrawPackages();
    

protected:
    // UBaseScreen overrides
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    // UUserWidget lifecycle
    virtual void NativeConstruct() override;

protected:
    // Button handlers
    UFUNCTION() void OnAcceptClicked();
    UFUNCTION() void OnCancelClicked();

    UFUNCTION() void OnTabSitClicked();
    UFUNCTION() void OnTabPkgClicked();
    UFUNCTION() void OnTabMapClicked();

    UFUNCTION() void OnSystemSelectChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION() void OnElemAddClicked();
    UFUNCTION() void OnElemEditClicked();
    UFUNCTION() void OnElemDelClicked();
    UFUNCTION() void OnElemIncClicked();
    UFUNCTION() void OnElemDecClicked();

    UFUNCTION() void OnEventAddClicked();
    UFUNCTION() void OnEventEditClicked();
    UFUNCTION() void OnEventDelClicked();
    UFUNCTION() void OnEventIncClicked();
    UFUNCTION() void OnEventDecClicked();

protected:
    // Legacy FORM text
    UPROPERTY(EditDefaultsOnly, Category = "FORM")
    FString LegacyFormText;

protected:
    // Manager / model
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> Manager = nullptr;

    Campaign* campaign = nullptr;
    Mission* mission = nullptr;
    MissionInfo* mission_info = nullptr;

    int32 current_tab = 0;

protected:
    // FORM IDs (tabs)
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnSit = nullptr; // 301
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnPkg = nullptr; // 302
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnMap = nullptr; // 303

    // OK/Cancel
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnAccept = nullptr; // 1
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnCancel = nullptr; // 2

    // Fields
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TxtName = nullptr;      // 201
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* CmbType = nullptr;      // 202
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* CmbSystem = nullptr;    // 203
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* CmbRegion = nullptr;    // 204

    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TxtDescription = nullptr; // 410
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TxtSituation = nullptr;   // 411
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* TxtObjective = nullptr;   // 412

    // Lists (NOTE: true column list requires row widget implementation)
    UPROPERTY(meta = (BindWidgetOptional)) UListView* LstElem = nullptr;  // 510
    UPROPERTY(meta = (BindWidgetOptional)) UListView* LstEvent = nullptr; // 520

    // Element buttons
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnElemAdd = nullptr; // 501
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnElemEdit = nullptr; // 505
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnElemDel = nullptr; // 502
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnElemInc = nullptr; // 503
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnElemDec = nullptr; // 504

    // Event buttons
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnEventAdd = nullptr; // 511
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnEventEdit = nullptr; // 515
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnEventDel = nullptr; // 512
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnEventInc = nullptr; // 513
    UPROPERTY(meta = (BindWidgetOptional)) UButton* BtnEventDec = nullptr; // 514
};
