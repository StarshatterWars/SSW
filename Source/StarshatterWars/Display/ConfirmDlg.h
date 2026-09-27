/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         ConfirmDlg.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves legacy FORM IDs and behavior.
    - Enter/Escape unified via UBaseScreen::HandleAccept/HandleCancel.
*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "ConfirmDlg.generated.h"

// UMG fwd:
class UButton;
class UTextBlock;

// Manager fwd (UE port):
class UMenuScreen;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmCanceled);

UCLASS()
class STARSHATTERWARS_API UConfirmDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UConfirmDlg(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------
    // UUserWidget lifecycle
    // ----------------------------------------------------------------
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    // ----------------------------------------------------------------
    // Legacy surface (ported)
    // ----------------------------------------------------------------
    virtual void RegisterControls();
    virtual void Show();

    // Manager
    void SetManager(UMenuScreen* InManager) { Manager = InManager; }

    // Classic API
    UFUNCTION(BlueprintCallable, Category = "ConfirmDlg")
    FString GetTitle() const;

    UFUNCTION(BlueprintCallable, Category = "ConfirmDlg")
    void SetTitle(const FString& InTitle);

    UFUNCTION(BlueprintCallable, Category = "ConfirmDlg")
    FString GetDialogMessage() const;

    UFUNCTION(BlueprintCallable, Category = "ConfirmDlg")
    void SetMessage(const FString& InMessage);

    // ----------------------------------------------------------------
    // Events
    // ----------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "ConfirmDlg")
    FOnConfirmAccepted OnConfirmed;

    UPROPERTY(BlueprintAssignable, Category = "ConfirmDlg")
    FOnConfirmCanceled OnCanceled;

protected:
    // ----------------------------------------------------------------
    // UBaseScreen overrides
    // ----------------------------------------------------------------
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    // Centralized Enter/Escape:
    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

protected:
    // ----------------------------------------------------------------
    // Callbacks (UMG)
    // ----------------------------------------------------------------
    UFUNCTION() void HandleApplyClicked();
    UFUNCTION() void HandleCancelClicked();

protected:
    // ----------------------------------------------------------------
    // Bound UMG controls (match FORM ids)
    // ----------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_apply = nullptr; // id 1
    UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_cancel = nullptr; // id 2

    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* lbl_title = nullptr; // id 100
    UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* lbl_message = nullptr; // id 101

protected:
    UPROPERTY(Transient)
    UMenuScreen* Manager = nullptr;
};
