/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionEditorNavDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionEditorNavDlg
    - UMG replacement for legacy MsnEditNavDlg (Navigation tab)
    - Inherits UBaseScreen (FORM ID binding + Enter/Escape policy)
    - Preserves legacy control IDs:
        1   Accept
        2   Cancel
        301 Situation tab
        302 Package tab
        303 Navigation tab (this)
        201 Mission name
        202 Mission type
        203 Star system
        204 Sector/region
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"

#include "MissionEditorNavDlg.generated.h"

class UMenuScreen;
class Mission;
class MissionInfo;
class StarSystem;

UCLASS()
class STARSHATTERWARS_API UMissionEditorNavDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionEditorNavDlg(const FObjectInitializer& ObjectInitializer);

    // Legacy API:
    virtual void RegisterControls();
    virtual void Show();
    virtual void ScrapeForm();

    void SetManager(UMenuScreen* InManager) { Manager = InManager; }
    void SetMission(Mission* InMission) { mission = InMission; }
    void SetMissionInfo(MissionInfo* InInfo) { mission_info = InInfo; }

protected:
    // UBaseScreen
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    virtual void NativeConstruct() override;

    // Enter/Escape behavior:
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

protected:
    // ------------------------------------------------------------
    // FORM-ID BINDINGS (optional UMG widgets)
    // ------------------------------------------------------------

    // Tabs:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnSit = nullptr; // 301
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnPkg = nullptr; // 302
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnMap = nullptr; // 303

    // Fields:
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableTextBox> TxtName = nullptr; // 201
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString>  CmbType = nullptr; // 202
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString>  CmbSystem = nullptr; // 203
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString>  CmbRegion = nullptr; // 204

private:
    void ShowTab(int32 TabIndex);

    void PopulateTypeOptionsIfEmpty();
    void PopulateSystems();
    void PopulateRegionsForSystem(StarSystem* Sys);
    StarSystem* FindSystemByName(const FString& Name) const;

private:
    // Click handlers:
    UFUNCTION() void OnCommitClicked();
    UFUNCTION() void OnCancelClicked();
    UFUNCTION() void OnTabClicked_Sit();
    UFUNCTION() void OnTabClicked_Pkg();
    UFUNCTION() void OnTabClicked_Map();

    // Selection handlers:
    UFUNCTION() void OnSystemSelect(FString SelectedItem, ESelectInfo::Type SelectionType);

private:
    UPROPERTY(Transient) TObjectPtr<UMenuScreen> Manager = nullptr;

    Mission* mission = nullptr;       // legacy core pointer
    MissionInfo* mission_info = nullptr;  // legacy core pointer

    // Local latch used to match old "exit_latch" behavior:
    bool bLocalExitLatch = false;
};


