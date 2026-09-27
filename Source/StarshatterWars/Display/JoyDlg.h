/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           JoyDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UJoyDlg
    - Joystick axis mapping subpage (OptionsScreen).
    - Auto-formats like Audio/Game/Controls by building rows into BaseScreen AutoVBox.
    - Player clicks an axis slot, then moves a physical joystick axis to detect it.
    - Saves to legacy KeyMap (key.cfg) and calls Stars->MapKeys().

    WBP CONTRACT (BindWidgetOptional)
    =================================
    Required widgets (Is Variable, names must match):
      - MessageText   (TextBlock)  OPTIONAL but recommended
      - AxisButton0..3 (Button)    REQUIRED
      - AxisText0..3   (TextBlock) OPTIONAL (typically the child text inside each button)
      - Invert0..3     (CheckBox)  OPTIONAL (recommended)
      - ApplyBtn / CancelBtn (Button) OPTIONAL if OptionsScreen owns Apply/Cancel
      - RootCanvas (CanvasPanel) OPTIONAL (only if you want AutoVBox injection)

    Optional local tab buttons (if present in page WBP):
      - VidTabButton / AudTabButton / CtlTabButton / OptTabButton / ModTabButton

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "JoyDlg.generated.h"

// Forward declarations:
class UButton;
class UTextBlock;
class UCheckBox;
class UHorizontalBox;
class UOptionsScreen;

UCLASS()
class STARSHATTERWARS_API UJoyDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UJoyDlg(const FObjectInitializer& ObjectInitializer);

    // Standardized across subscreens:
    void SetOptionsManager(UOptionsScreen* InManager) { OptionsManager = InManager; }
    UOptionsScreen* GetOptionsManager() const { return OptionsManager.Get(); }

    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    virtual void ExecFrame(double DeltaTime) override;

    void Show();
    void Apply();
    void Cancel();

protected:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    void BindDelegates();

    // AutoVBox row building:
    void BuildJoyRows();
    void AttachTextToButton(UButton* Button, UTextBlock* TextWidget);
    void BuildAxisRow(const FString& LabelText, UButton* AxisButton, UCheckBox* InvertCheck, const FName& RowName);

    // Behavior:
    void HandleAxisClicked(int32 AxisIndex);
    void RefreshAxisUIFromCurrentBindings();
    void SetAxisLabel(int32 AxisIndex, const TCHAR* InText);

    void CommitToKeyMap();

private:
    bool bDelegatesBound = false;

    int32 SelectedAxis = -1;   // slot waiting for input (0..3)
    int32 SampleAxis = -1;   // last detected physical axis (0..7)

    int32 Samples[8] = { 0,0,0,0,0,0,0,0 };
    int32 MapAxis[4] = { -1,-1,-1,-1 }; // slot->physical axis (0..7), -1 unmapped

protected:
    // Status text (optional)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MessageText = nullptr;

    // Axis buttons (required)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AxisButton0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AxisButton1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AxisButton2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AxisButton3 = nullptr;

    // Axis label TextBlocks (optional; typically the child text inside each button)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AxisText0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AxisText1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AxisText2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AxisText3 = nullptr;

    // Invert checkboxes (recommended)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Invert0 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Invert1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Invert2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> Invert3 = nullptr;

    // Apply / Cancel (optional if OptionsScreen owns global Apply/Cancel)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn = nullptr;

    // Optional local tabs
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> VidTabButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AudTabButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CtlTabButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OptTabButton = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ModTabButton = nullptr;

private:
    // Apply/Cancel
    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();

    // Axis slots
    UFUNCTION() void OnAxis0Clicked();
    UFUNCTION() void OnAxis1Clicked();
    UFUNCTION() void OnAxis2Clicked();
    UFUNCTION() void OnAxis3Clicked();

    // Tabs
    UFUNCTION() void OnAudioClicked();
    UFUNCTION() void OnVideoClicked();
    UFUNCTION() void OnControlsClicked();
    UFUNCTION() void OnOptionsClicked();
    UFUNCTION() void OnModClicked();
};
