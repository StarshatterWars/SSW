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
    - Operational Command / Forces tab.
    - Uses a single hierarchical ListView rooted at the selected combatant force.
    - Dropdown options are mapped directly to combatants and display:
      <Empire Name>: <Force Name>
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

UCLASS()
class STARSHATTERWARS_API UCmdForceDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    FString GetEmpireDisplayName(EEMPIRE_NAME Empire) const;
    UCmdForceDlg(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
    void SetManager(UCmpnScreen* InManager);
    void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
    void ShowForceDlg();
    void ExecFrame();
    void SetModeAndHighlight(ECOMMAND_MODE InMode);

private:
    UFUNCTION()
    void OnForceSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnTransferClicked();

    UFUNCTION()
    void OnCombatItemSelected(UObject* ItemObject);

private:
    bool IsVisibleCombatant(Combatant* C) const;
    void ShowCombatant(Combatant* C);
    void RebuildCombatListForCurrentCombatant();
    void AddCombatGroupRecursive(CombatGroup* Group, bool bLastChild, int32 Depth);
    bool CanTransfer(CombatGroup* Group) const;

    void ClearDescList();
    void PopulateDescForGroup(CombatGroup* Group);
    void PopulateDescForUnit(CombatUnit* Unit);

    void UpdateTransferEnabled();
    void UpdateTransferButtonState();

    void PopulateForcesComboBox();
    FString BuildCombatantDropdownLabel(Combatant* C) const;
    Combatant* ResolveCombatantFromDropdownLabel(const FString& SelectedItem) const;

private:
    UCmpnScreen* Manager = nullptr;

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    CombatGroup* CurrentGroup = nullptr;
    CombatUnit* CurrentUnit = nullptr;
    Combatant* CurrentCombatant = nullptr;

    bool bBlankLine = false;

    TMap<FString, Combatant*> DropdownCombatantMap;

private:
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UComboBoxString* ForcesComboBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UListView* CombatantList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UListView* DescList = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UButton* TransferButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* TransferButtonText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupTypeText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupLocationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupEmpireText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* GroupInfoText = nullptr;

private:
    ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_FORCES;

protected:
    UPROPERTY()
    UCmdDlg* ParentCmdDlg = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UCmdMsgDlg* CmdMsgDlg = nullptr;

    void ShowTransferPopup(const FString& Title, const FString& Message, bool bApproved);

    /*
    // ---------------------------------------------------------------------
    // OLD OOB / PIPE-STACK HELPERS (DISABLED)
    // ---------------------------------------------------------------------
    CombatGroup* GetTopForceGroup(CombatGroup* Group) const;
    void DumpCombatGroupRecursive(CombatGroup* Group, int32 Depth);

    FString PipeStack;
    */
};