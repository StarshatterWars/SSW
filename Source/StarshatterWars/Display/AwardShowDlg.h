/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         AwardShowDlg.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UAwardShowDlg
    - Unreal port of Starshatter AwardShowDlg (rank/medal detail popup).
    - Uses UBaseScreen FORM-id binding and optional .frm parsing defaults.
    - Binds:
        203 = Name/Title label
        201 = Info/Description label
        202 = Insignia image (rank/medal)
        1   = Close button
    - Enter/Escape: closes only after latch released (legacy behavior).
    - Close returns to manager->ShowPlayerDlg() (MenuScreen equivalent).
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"

#include "AwardShowDlg.generated.h"

class UTextBlock;
class UButton;
class UImage;

UCLASS()
class STARSHATTERWARS_API UAwardShowDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UAwardShowDlg(const FObjectInitializer& ObjectInitializer);

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
    /** Show dialog (calls ShowAward and latches input) */
    UFUNCTION(BlueprintCallable)
    void ShowDialog();

    /** Equivalent to legacy SetRank(int r) */
    UFUNCTION(BlueprintCallable)
    void SetRank(int32 InRank);

    /** Equivalent to legacy SetMedal(int m) */
    UFUNCTION(BlueprintCallable)
    void SetMedal(int32 InMedal);

    /** Refreshes the dialog UI based on Rank/Medal selection */
    UFUNCTION(BlueprintCallable)
    void ShowAward();

    /** Set the menu manager / owner (MenuScreen equivalent) */
    UFUNCTION(BlueprintCallable)
    void SetManager(UBaseScreen* InManager);

protected:
    UFUNCTION()
    void OnCloseClicked();

private:
    void UpdateExitLatchFromInput(const FKeyEvent& InKeyEvent, bool& bOutHandled);

private:
    // ------------------------------------------------------------
    // Manager / owner
    // ------------------------------------------------------------
    UPROPERTY(Transient)
    TObjectPtr<UBaseScreen> Manager = nullptr;

    // ------------------------------------------------------------
    // FORM bound widgets
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

    // ------------------------------------------------------------
    // State
    // -----------------------------------------------------------
    UPROPERTY(Transient)
    int32 Rank = -1;

    UPROPERTY(Transient)
    int32 Medal = -1;
};
