/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionElementDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    UMissionElementDlg
    - Unreal UUserWidget replacement for legacy MsnElemDlg
    - Inherits from UBaseScreen (legacy FORM support + Enter/Escape behavior)
    - Uses standard UMG widget bindings (BindWidgetOptional)
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "Math/Vector.h"              // FVector
#include "Math/Color.h"               // FColor
#include "Math/UnrealMathUtility.h"  

#include "GameStructs.h"// Math

#include "MissionElementDlg.generated.h"

// Forward declarations (keep header light)
class UButton;
class UComboBoxString;
class UEditableTextBox;

class UMenuScreen;

class Mission;
class MissionElement;

UCLASS()
class STARSHATTERWARS_API UMissionElementDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionElementDlg(const FObjectInitializer& ObjectInitializer);

    void InitializeDlg(UMenuScreen* InManager);
    void SetMission(Mission* InMission);
    void SetMissionElement(MissionElement* InElem);

    void ShowDlg();

protected:
    // UBaseScreen overrides
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    // UUserWidget lifecycle
    virtual void NativeConstruct() override;

protected:
    void RebuildFromModel();
    void RebuildDesignListFromClass();
    void RebuildSkinAndLoadoutFromDesign();
    void BuildObjectiveTargets();
    void UpdateTeamInfo();

    bool CanCommand(const MissionElement* Commander, const MissionElement* Subordinate) const;

protected:
    // UI events
    UFUNCTION() void OnClassChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnDesignChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnObjectiveChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION() void OnIFFCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION() void OnAcceptClicked();
    UFUNCTION() void OnCancelClicked();

protected:
    // Legacy FORM text (optional: paste .frm text here or set in editor)
    UPROPERTY(EditDefaultsOnly, Category = "FORM")
    FString LegacyFormText;

protected:
    // Manager / model (legacy classes are non-UObject)
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> Manager = nullptr;

    Mission* MissionPtr = nullptr;
    MissionElement* ElemPtr = nullptr;

    bool bExitLatch = false;

protected:
    // Buttons
    UPROPERTY(meta = (BindWidgetOptional)) UButton* AcceptButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UButton* CancelBtn = nullptr;

protected:
    // Combos
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* ClassCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* DesignCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* SkinCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* LoadoutCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* RoleCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* RegionCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* HeadingCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* IntelCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* ObjectiveCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* TargetCombo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* CommanderCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* SquadronCombo = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UComboBoxString* CarrierCombo = nullptr;

protected:
    // Text fields
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* NameEdit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* SizeEdit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* IFFEdit = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* LocXEdit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* LocYEdit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* LocZEdit = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* RespawnsEdit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) UEditableTextBox* HoldTimeEdit = nullptr;


};
