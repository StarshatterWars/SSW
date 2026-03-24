/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionObjectiveDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    MissionObjectiveDlg
    - Unreal conversion of MsnObjDlg (Mission Briefing: Situation/Objectives + beauty/skin).
    - FORM-driven via UBaseScreen::ParseLegacyForm + ApplyLegacyFormDefaults.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MissionObjectiveDlg.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;

UCLASS()
class STARSHATTERWARS_API UMissionObjectiveDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionObjectiveDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // Legacy-style operations
    // ----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "MissionObjectiveDlg")
    void ExecFrame(float DeltaSeconds);

    // ----------------------------------------------------------------
    // UBaseScreen overrides
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

protected:
    virtual void NativeConstruct() override;

    // Enter/Escape behavior
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    // ----------------------------------------------------------------
    // UI callbacks
    // ----------------------------------------------------------------
    UFUNCTION()
    void OnClickedAccept();

    UFUNCTION()
    void OnClickedCancel();

    UFUNCTION()
    void OnClickedTabSit();

    UFUNCTION()
    void OnClickedTabPkg();

    UFUNCTION()
    void OnSelectionChangedSkin(FString SelectedItem, ESelectInfo::Type SelectionType);

private:
    // ----------------------------------------------------------------
    // Control IDs (match FORM)
    // ----------------------------------------------------------------
    static constexpr int32 ID_ACCEPT = 1;
    static constexpr int32 ID_CANCEL = 2;

    static constexpr int32 ID_TAB_SIT = 900;
    static constexpr int32 ID_TAB_PKG = 901;
    static constexpr int32 ID_TAB_MAP = 902;
    static constexpr int32 ID_TAB_WEP = 903;

    static constexpr int32 ID_OBJECTIVES = 400;
    static constexpr int32 ID_SITREP = 401;

    static constexpr int32 ID_BEAUTY = 300; // in legacy was a 3D view; keep as label/placeholder for now
    static constexpr int32 ID_PLAYER_DESC = 301;
    static constexpr int32 ID_SKIN_COMBO = 302;

private:
    // ----------------------------------------------------------------
    // Bound widgets (optional – you can also just use FormMap lookup by ID)
    // ----------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AcceptButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelButtonW = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> TabSitButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> TabPkgButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> SkinCombo = nullptr;
};
