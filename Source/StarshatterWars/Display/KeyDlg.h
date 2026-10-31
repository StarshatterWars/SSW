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
    - Clear restores the CURRENT selected action default (staged until Apply).

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


class UButton;
class UTextBlock;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UOptionsScreen;
class UInputMappingContext;
#include "EnhancedActionKeyMapping.h"
#include "KeyDlg.generated.h"

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
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event) override;

    // BaseScreen polling hook (kept for parity; no polling required)
    virtual void ExecFrame(double DeltaTime) override;

    void Apply();
    void Cancel();

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
    void AppendFighterHUDBindings();
    UVerticalBox* CategoryBox(const FString& Category);
    bool CaptureKey(FKey Key);

    // Model/UI
    void RefreshDisplayFromSettings();
    void CommitPendingToSettings();

    // Selection/capture
    void BeginCaptureForSelected();

    // Styling
    void ApplyStandardValueTextStyle(UTextBlock* Block) const;

    // Helpers

private:
    // One handler for all row buttons (pressed is deterministic via IsPressed())
    UFUNCTION() void OnRowButtonPressed();

    // Buttons
    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();
    UFUNCTION() void OnClearClicked();

private:
    bool bDelegatesBound = false;

    friend class FKeyboardRemappingTest;
    bool bCapturing = false;
    FString SelectedEnhancedId;
    double CaptureStarted=0;
    FVector2D CaptureMouseDelta=FVector2D::ZeroVector;
    TMap<FString,FKey> PendingEnhanced;
    TSet<FString> PendingEnhancedClears;
    TMap<FString,TWeakObjectPtr<UVerticalBox>> CategoryBoxes;
    struct FEnhancedRow
    {
        FEnhancedActionKeyMapping Mapping;
        TObjectPtr<UButton> Button=nullptr;
        TObjectPtr<UTextBlock> KeyText=nullptr;
    };
    TMap<FString,FEnhancedRow> EnhancedRows;
    UPROPERTY(Transient) TArray<TObjectPtr<UInputMappingContext>> BindingContexts;

    bool bHasSelection = false;

    // Pending edits staged until Apply:

    // Scroll list widgets we construct
    UPROPERTY(Transient) TObjectPtr<UScrollBox>  KeyScrollBox = nullptr;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyListVBox = nullptr;

    TWeakObjectPtr<UButton> SelectedRowButton;

protected:
    // Optional BP buttons (recommended)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ClearButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn = nullptr;
};
