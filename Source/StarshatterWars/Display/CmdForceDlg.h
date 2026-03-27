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
    - In legacy, CmdForceDlg : FormWindow, CmdDlg. In Unreal we do NOT multiply inherit.
      Instead, this widget *contains* the CmdDlg header widgets (same IDs) and routes mode/save/exit
      back to UCmpnScreen.
    - Maintains the same data flows:
        * Forces ComboBox (combatants)
        * Combat tree list (groups + units, expandable)
        * Description list (key/value rows)
        * Transfer button (approval/denial -> CmdMsgDlg)
*/

#pragma once

#include "CoreMinimal.h"
#include "CmdDlg.h"          // for ECmdMode / shared behavior patterns
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

/**
 * Forces Tab (Order of Battle)
 */
UCLASS()
class STARSHATTERWARS_API UCmdForceDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UCmdForceDlg(const FObjectInitializer& ObjectInitializer);

    // ============================================================
    // Lifecycle
    // ============================================================
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    // ============================================================
    // UBaseScreen overrides (FORM binding)
    // ============================================================
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    // ============================================================
    // Public API (legacy equivalents)
    // ============================================================
public:
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowForceDlg();
    void ExecFrame();

private:
    // ============================================================
    // Tab routing (Mode/Save/Exit)
    // ============================================================
    void SetModeAndHighlight(ECOMMAND_MODE InMode);

    // ============================================================
    // Forces / Combat list interactions
    // ============================================================
    UFUNCTION()
    void OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnTransferClicked();

private:
    // ============================================================
    // Core logic (ported)
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

private:
    // ============================================================
    // Manager / dependencies
    // ============================================================
    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;
    CombatGroup* CurrentGroup = nullptr;
    CombatUnit* CurrentUnit = nullptr;
    Combatant* CurrentCombatant = nullptr;

private:
    // ============================================================
    // Forces tab widgets (IDs 400..403)
    // ============================================================
    UPROPERTY(meta = (BindWidgetOptional), Transient) UComboBoxString* cmb_forces = nullptr; // 400

    // For UMG, use ListView entries instead of legacy ListBox:
    UPROPERTY(meta = (BindWidgetOptional), Transient) UListView* lst_combat = nullptr; // 401
    UPROPERTY(meta = (BindWidgetOptional), Transient) UListView* lst_desc = nullptr; // 402

    UPROPERTY(meta = (BindWidgetOptional), Transient) UButton* btn_transfer = nullptr; // 403

private:
    // ============================================================
    // Internal formatting state (legacy pipe stack)
    // ============================================================
    FString PipeStack;
    bool bBlankLine = false;

    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_FORCES;

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;
};
