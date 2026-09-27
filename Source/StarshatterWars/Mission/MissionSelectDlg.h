/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionSelectDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionSelectDlg (Unreal)
    - Full conversion of legacy MsnSelectDlg into a UBaseScreen-derived widget.
    - Uses FORM id binding (BaseScreen) and legacy .frm parsing behavior.
    - Populates campaign list (ComboBox or ListView) and mission list (UListView).
    - Uses UMissionListItem as the UObject row model for mission entries.
    - Preserves legacy logic:
        * Campaign selection updates mission list
        * Mission selection updates description RichText and enables Accept
        * Accept reloads mission and enters PREP_MODE
        * Cancel returns to parent via delegate
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionSelectDlg.generated.h"

// Forward declarations (legacy core):
class Starshatter;
class Campaign;
class Mission;

// Forward declarations (Unreal UI row model):
class UMissionListItem;

// Starshatter fwd:
class UMenuScreen;

/**
 * Mission Select dialog (legacy MsnSelectDlg).
 * FORM ids should match legacy .frm:
 *   1   Accept (button)
 *   2   Cancel (button)
 *   200 Description (text / RichText)
 *   201 Campaign Combo (combo)   [optional]
 *   203 Campaign List  (list)    [optional]
 *   202 Mission List   (list)
 *   301 New  (button)  [optional]
 *   302 Edit (button)  [optional]
 *   303 Delete(button) [optional]
 */
UCLASS()
class STARSHATTERWARS_API UMissionSelectDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionSelectDlg(const FObjectInitializer& ObjectInitializer);

    // Legacy flow:
    virtual void Show() override;
    virtual void ExecFrame(double DeltaTime) override;

    // UBaseScreen Enter/Escape:
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

    // Menu manager hookup (matches your existing pattern)
    virtual void SetMenuManager(UMenuScreen* InManager);
    virtual void InitializeDlg(UMenuScreen* InManager);

    // Buttons
    UButton* BtnAccept = nullptr;
    UButton* BtnCancel = nullptr;
    UButton* BtnNew = nullptr;
    UButton* BtnEdit = nullptr;
    UButton* BtnDelete = nullptr;

    // Lists / Combo
    UListView* MissionsList = nullptr;
    UListView* CampaignsList = nullptr;
    UComboBoxString* CampaignCombo = nullptr;

    // Description text
    URichTextBlock* DescriptionText = nullptr;

    // External hook points (so we don't hard-couple to a specific menu screen):
    DECLARE_MULTICAST_DELEGATE(FOnMissionSelectDlgCancelled);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnMissionSelectDlgAccepted, int32 /*MissionId*/);

    FOnMissionSelectDlgCancelled OnCancelled;
    FOnMissionSelectDlgAccepted  OnAccepted;

protected:
    // UBaseScreen binding:
    virtual void BindFormWidgets() override;

protected:
    // UUserWidget lifecycle:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UMenuScreen> manager = nullptr;

protected:
    // Wiring (no lambdas):
    void WireEvents();

    // Legacy operations:
    void RegisterControls();
    void PopulateCampaigns();
    void PopulateMissions();
    void UpdateDescriptionAndButtons();

    // Event handlers:
    UFUNCTION() void OnAcceptClicked();
    UFUNCTION() void OnCancelClicked();

    UFUNCTION() void OnNewClicked();
    UFUNCTION() void OnEditClicked();
    UFUNCTION() void OnDeleteClicked();

    // Campaign selection handlers:
    UFUNCTION() void OnCampaignComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void OnCampaignListSelectionChanged(UObject* SelectedItem);

    // Mission selection handler:
    UFUNCTION() void OnMissionListSelectionChanged(UObject* SelectedItem);

protected:
    // Legacy core pointers:
    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    // Legacy state:
    int32 SelectedMissionIndex = -1;
    int32 MissionId = 0;
    bool  bEditable = false;

    // Cache edit mission selection (legacy static):
    static Mission* EditMission;

protected:
    // Cached list items (owned by UE GC once added to arrays):
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMissionListItem>> MissionItems;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> CampaignListItems; // optional, only if you implement campaign list as UObject items

private:
    bool bWired = false;
};
