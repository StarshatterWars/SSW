/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           ControlOptionsDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UControlOptionsDlg
    - Controls settings subpage hosted by UOptionsScreen.
    - Auto-injects an "AutoVBox" into RootCanvas with standard margins:
        Top = 64, Left/Right/Bottom = 32, fills panel.
    - Builds runtime rows (label + control) using BaseScreen::AddLabeledRow(...)

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "ControlOptionsDlg.generated.h"

// UMG
class UButton;
class UComboBoxString;
class USlider;
class UCheckBox;

class UOptionsScreen;
class UStarshatterControlsSettings;

UCLASS()
class STARSHATTERWARS_API UControlOptionsDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UControlOptionsDlg(const FObjectInitializer& ObjectInitializer);

    void Show();
    virtual void ExecFrame(double DeltaTime) override;

    void Apply();
    void Cancel();

    void PushToModel(bool bApplyRuntimeToo);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    virtual void BindFormWidgets() override;
    virtual FString GetLegacyFormText() const override;

    virtual void HandleAccept() override;
    virtual void HandleCancel() override;

private:
    void BindDelegates();

    UStarshatterControlsSettings* GetControlsSettings() const;

    void RefreshFromModel();

    void BuildControlRows();
    void BuildControlModelListIfNeeded();

    static int32 SliderToInt(float Normalized, int32 MinV, int32 MaxV);
    static float IntToSlider(int32 V, int32 MinV, int32 MaxV);

private:
    bool bClosed = true;
    bool bDelegatesBound = false;
    bool bDirty = false;

    // Snapshot mirrors FStarshatterControlsConfig
    int32 ControlModel = 1; // FlightSim default

    int32 JoystickIndex = 0;
    int32 ThrottleAxis = 0;
    int32 RudderAxis = 0;
    int32 JoystickSensitivity = 5;

    int32 MouseSensitivity = 25;
    bool  bMouseInvert = false;

protected:
    // ------------------------------------------------------------
    // Controls (bound from BP, inserted into runtime rows)
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> ControlModelCombo;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> JoystickIndexSlider;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> ThrottleAxisSlider;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> RudderAxisSlider;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> JoystickSensitivitySlider;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> MouseSensitivitySlider;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> MouseInvertCheck;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ApplyBtn;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelBtn;

    // Optional local tabs (only if present in the page WBP)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> VidTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> AudTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CtlTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OptTabButton;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ModTabButton;

private:
    // Model change handlers
    UFUNCTION() void OnControlModelChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION() void OnJoystickIndexChanged(float V);
    UFUNCTION() void OnThrottleAxisChanged(float V);
    UFUNCTION() void OnRudderAxisChanged(float V);
    UFUNCTION() void OnJoystickSensitivityChanged(float V);

    UFUNCTION() void OnMouseSensitivityChanged(float V);
    UFUNCTION() void OnMouseInvertChanged(bool bIsChecked);

    UFUNCTION() void OnApplyClicked();
    UFUNCTION() void OnCancelClicked();

    // Tabs
    UFUNCTION() void OnAudioClicked();
    UFUNCTION() void OnVideoClicked();
    UFUNCTION() void OnOptionsClicked();
    UFUNCTION() void OnControlsClicked();
    UFUNCTION() void OnModClicked();
};
