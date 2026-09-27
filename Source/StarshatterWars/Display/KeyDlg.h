/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           KeyDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UKeyDlg
    - Full keybind list view INSIDE KeyDlg (scrollable).
    - AutoVBox-driven layout (like Audio/Game/Controls).
    - Click a row to select an action, then press a key to stage a new bind.
    - Apply commits ALL pending changes to UStarshatterKeyboardSettings (config-backed CDO)
      and applies to runtime via UStarshatterKeyboardSubsystem.
    - Clear removes the CURRENT selected action binding (staged until Apply).

    IMPORTANT
    =========
    - Uses AddUniqueDynamic (NO RemoveAll) to prevent ensure failures.
    - Everything is built into AutoVBox (no floating widgets at 0,0).
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "InputCoreTypes.h"

#include "GameStructs.h" // EStarshatterInputAction
#include "KeyDlg.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UOptionsScreen;

UCLASS()
class STARSHATTERWARS_API UKeyDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UKeyDlg(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Key capture:
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    // BaseScreen polling hook (kept for parity; no polling required)
    virtual void ExecFrame(double DeltaTime) override;

    void SetOptionsManager(UOptionsScreen* InManager) { OptionsManager = InManager; }

protected:
    virtual void BindFormWidgets() override {}
    virtual FString GetLegacyFormText() const override { return FString(); }

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    void BindDelegatesOnce();

    // Layout/build
    void BuildKeyRows();
    void BuildKeyList();

    // Model/UI
    void RefreshDisplayFromSettings();
    void CommitPendingToSettings();

    // Selection/capture
    void SelectAction(EStarshatterInputAction Action);
    void BeginCaptureForSelected();

    // Styling
    void ApplyStandardValueTextStyle(UTextBlock* Block) const;

    // Helpers
    static FString ActionToDisplayString(EStarshatterInputAction Action);
    static FString KeyToDisplayString(const FKey& Key);
    static bool IsModifierKey(const FKey& Key);

private:
    // One handler for all row buttons (pressed is deterministic via IsPressed())
    UFUNCTION() void OnRowButtonPressed();

    // Buttons
    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();
    UFUNCTION() void OnClearClicked();

private:
    bool bDelegatesBound = false;

    bool bCapturing = false;

    EStarshatterInputAction SelectedAction = EStarshatterInputAction::Pause;
    bool bHasSelection = false;

    // Pending edits staged until Apply:
    TMap<EStarshatterInputAction, FKey> PendingRemaps;
    TSet<EStarshatterInputAction> PendingClears;

    // Scroll list widgets we construct
    UPROPERTY(Transient) TObjectPtr<UScrollBox>  KeyScrollBox = nullptr;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyListVBox = nullptr;

    struct FKeyRowWidgets
    {
        TObjectPtr<UButton>    RowButton = nullptr;
        TObjectPtr<UTextBlock> ActionText = nullptr;
        TObjectPtr<UTextBlock> KeyText = nullptr;
    };

    TMap<EStarshatterInputAction, FKeyRowWidgets> RowMap;
    TMap<TWeakObjectPtr<UButton>, EStarshatterInputAction> ButtonToAction;

    TWeakObjectPtr<UButton> SelectedRowButton;

protected:
    // Optional BP buttons (recommended)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ClearButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn = nullptr;
};
