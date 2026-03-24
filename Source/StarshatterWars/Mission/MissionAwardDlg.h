/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionAwardDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UAwardDlg
    - Unreal port of Starshatter AwardDlg (award popup).
    - Uses UBaseScreen FORM-id binding and optional .frm parsing defaults.
    - Binds:
        203 = Award Name (Label)
        201 = Award Description (Label)
        202 = Award Insignia (Image)
        1   = Close (Button)
    - On open: ShowPlayer()
    - Enter/Escape: closes (via UBaseScreen default dialog handling or local handler)
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "MissionAwardDlg.generated.h"

class UTextBlock;
class UButton;
class UImage;

UCLASS()
class STARSHATTERWARS_API UMissionAwardDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMissionAwardDlg(const FObjectInitializer& ObjectInitializer);

    // ------------------------------------------------------------
    // UUserWidget lifecycle
    // ------------------------------------------------------------
protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // ------------------------------------------------------------
    // UBaseScreen overrides
    // ------------------------------------------------------------
public:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    // ------------------------------------------------------------
    // Starshatter behavior (ported API)
    // ------------------------------------------------------------
public:
    /** Called by manager when dialog is shown/opened */
    UFUNCTION(BlueprintCallable)
    void ShowDialog();

    /** Refreshes UI from current player award fields */
    UFUNCTION(BlueprintCallable)
    void ShowPlayer();

protected:
    /** Close button click / Enter / Escape */
    UFUNCTION()
    void OnCloseClicked();

private:
    /** Prevent immediate Enter/Escape close on first frame (matches exit_latch) */
    void UpdateExitLatchFromInput(const FKeyEvent& InKeyEvent, bool& bOutHandled);

private:
    // ------------------------------------------------------------
    // Manager / dependencies
    // ------------------------------------------------------------
    // In the legacy code this is PlanScreen* manager.
    // In Unreal, we keep a weak pointer to whatever screen owns it (often a plan/menu screen widget).
    UPROPERTY(Transient)
    TObjectPtr<UBaseScreen> Manager = nullptr;

    // ------------------------------------------------------------
    // FORM bound widgets (by ID)
    // ------------------------------------------------------------
    // 203
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* lbl_name = nullptr;

    // 201
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UTextBlock* lbl_info = nullptr;

    // 202
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UImage* img_rank = nullptr;

    // 1
    UPROPERTY(meta = (BindWidgetOptional), Transient)
    UButton* btn_close = nullptr;
};
