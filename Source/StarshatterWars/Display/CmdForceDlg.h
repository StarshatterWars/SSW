/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         CmdForceDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmdForceDlg
    - Unreal port of CmdForceDlg (Operational Command / Forces tab).
*/

#pragma once

#include "CoreMinimal.h"
#include "CmdDlg.h"
#include "BaseScreen.h"
#include "Campaign.h"
#include "Starshatter.h"
#include "GameStructs.h"

#include "CmdForceDlg.generated.h"

class UComboBoxString;
class UButton;
class UListView;
class UTextBlock;

class Campaign;
class Combatant;
class CombatGroup;
class CombatUnit;
class Starshatter;

class UCmpnScreen;
class UCmdMsgDlg;
class UCmdDlg;
class UCmdForceListItem;

// ============================================================
// Forces Tab (Order of Battle)
// ============================================================


UCLASS()
class STARSHATTERWARS_API UCmdForceDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdForceDlg(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:

    // ============================================================
    // Public API
    // ============================================================
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowForceDlg();
    void ExecFrame();

    void SetModeAndHighlight(ECOMMAND_MODE InMode);

private:
    // ============================================================
    // UI Events
    // ============================================================

    UFUNCTION()
    void OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnTransferClicked();

    CombatGroup* GetTopForceGroup(CombatGroup* Group) const;

    void DumpCombatGroupRecursive(CombatGroup* Group, int32 Depth);

    UFUNCTION()
    void OnCombatItemSelected(UObject* ItemObject);

private:
    // ============================================================
    // Core Logic
    // ============================================================

    bool IsVisibleCombatant(Combatant* C) const;
    void ShowCombatant(Combatant* C);
    void RebuildCombatListForCurrentCombatant();
    void AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild);

    bool CanTransfer(CombatGroup* Group) const;

    void ClearDescList();
    void PopulateDescForGroup(CombatGroup* Group);
    void PopulateDescForUnit(CombatUnit* Unit);

    void UpdateTransferEnabled();

    void PopulateForcesComboBox();

private:
    // ============================================================
    // Manager / Dependencies
    // ============================================================

    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    CombatGroup* CurrentGroup = nullptr;
    CombatUnit* CurrentUnit = nullptr;
    Combatant* CurrentCombatant = nullptr;

private:
    // ============================================================
    // UI Bindings
    // ============================================================

    // Forces tab controls
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UComboBoxString* ForcesComboBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UListView* CombatantList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UListView* DescList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UButton* TransferButton = nullptr;

    // ------------------------------------------------------------
    // Description panel (RIGHT SIDE)
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupInfoText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupTypeText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupLocationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupEmpireText = nullptr;

private:
    // ============================================================
    // Internal state (tree formatting)
    // ============================================================

    FString PipeStack;
    bool bBlankLine = false;

    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_FORCES;

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;
};