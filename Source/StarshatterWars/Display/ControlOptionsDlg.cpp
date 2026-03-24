/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           ControlOptionsDlg.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UControlOptionsDlg

    Notes:
    - Delegates are bound once via AddUniqueDynamic (no RemoveAll needed).
    - AutoVBox runtime layout (EnsureAutoVerticalBox + AddLabeledRow).
    - OptionsScreen is the router; this page forwards if OptionsManager exists.
=============================================================================*/

#include "ControlOptionsDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/VerticalBox.h"

// Model
#include "StarshatterControlsSettings.h"

// Router
#include "OptionsScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogControlOptionsDlg, Log, All);

UControlOptionsDlg::UControlOptionsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UControlOptionsDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Enter/Escape routing (AudioDlg pattern)
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();
}

void UControlOptionsDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    EnsureAutoVerticalBox();
    if (AutoVBox)
        AutoVBox->ClearChildren();
}

void UControlOptionsDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegates();

    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
    {
        UE_LOG(LogControlOptionsDlg, Error, TEXT("[ControlOptionsDlg] BuildRows FAILED: AutoVBox is NULL"));
        return;
    }

    VBox->SetVisibility(ESlateVisibility::Visible);

    BuildControlModelListIfNeeded();
    BuildControlRows();

    UE_LOG(LogControlOptionsDlg, Warning, TEXT("[ControlOptionsDlg] AutoVBox children after BuildControlRows: %d"),
        VBox->GetChildrenCount());

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

void UControlOptionsDlg::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Apply/Cancel
    if (ApplyBtn)  ApplyBtn->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnApplyClicked);
    if (CancelBtn) CancelBtn->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnCancelClicked);

    // Optional local tabs
    if (AudTabButton) AudTabButton->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnAudioClicked);
    if (VidTabButton) VidTabButton->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnVideoClicked);
    if (CtlTabButton) CtlTabButton->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnControlsClicked);
    if (OptTabButton) OptTabButton->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnOptionsClicked);
    if (ModTabButton) ModTabButton->OnClicked.AddUniqueDynamic(this, &UControlOptionsDlg::OnModClicked);

    // Model controls
    if (ControlModelCombo)
        ControlModelCombo->OnSelectionChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnControlModelChanged);

    if (JoystickIndexSlider)
        JoystickIndexSlider->OnValueChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnJoystickIndexChanged);

    if (ThrottleAxisSlider)
        ThrottleAxisSlider->OnValueChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnThrottleAxisChanged);

    if (RudderAxisSlider)
        RudderAxisSlider->OnValueChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnRudderAxisChanged);

    if (JoystickSensitivitySlider)
        JoystickSensitivitySlider->OnValueChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnJoystickSensitivityChanged);

    if (MouseSensitivitySlider)
        MouseSensitivitySlider->OnValueChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnMouseSensitivityChanged);

    if (MouseInvertCheck)
        MouseInvertCheck->OnCheckStateChanged.AddUniqueDynamic(this, &UControlOptionsDlg::OnMouseInvertChanged);

    bDelegatesBound = true;
}

void UControlOptionsDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame((double)InDeltaTime);
}

void UControlOptionsDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling required.
}

void UControlOptionsDlg::BindFormWidgets() {}
FString UControlOptionsDlg::GetLegacyFormText() const { return FString(); }

void UControlOptionsDlg::HandleAccept() { OnApplyClicked(); }
void UControlOptionsDlg::HandleCancel() { OnCancelClicked(); }

void UControlOptionsDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

// ---------------- Model ----------------

UStarshatterControlsSettings* UControlOptionsDlg::GetControlsSettings() const
{
    return UStarshatterControlsSettings::Get();
}

void UControlOptionsDlg::RefreshFromModel()
{
    UStarshatterControlsSettings* Settings = GetControlsSettings();
    if (!Settings)
        return;

    Settings->Load();
    const FStarshatterControlsConfig& C = Settings->GetControlsConfig();

    ControlModel = (int32)C.ControlModel;

    JoystickIndex = C.JoystickIndex;
    ThrottleAxis = C.ThrottleAxis;
    RudderAxis = C.RudderAxis;
    JoystickSensitivity = C.JoystickSensitivity;

    MouseSensitivity = C.MouseSensitivity;
    bMouseInvert = C.bMouseInvert;

    if (ControlModelCombo)
        ControlModelCombo->SetSelectedIndex(FMath::Clamp(ControlModel, 0, 2));

    if (JoystickIndexSlider)       JoystickIndexSlider->SetValue(IntToSlider(JoystickIndex, 0, 8));
    if (ThrottleAxisSlider)        ThrottleAxisSlider->SetValue(IntToSlider(ThrottleAxis, 0, 16));
    if (RudderAxisSlider)          RudderAxisSlider->SetValue(IntToSlider(RudderAxis, 0, 16));
    if (JoystickSensitivitySlider) JoystickSensitivitySlider->SetValue(IntToSlider(JoystickSensitivity, 0, 10));

    if (MouseSensitivitySlider) MouseSensitivitySlider->SetValue(IntToSlider(MouseSensitivity, 0, 50));
    if (MouseInvertCheck)       MouseInvertCheck->SetIsChecked(bMouseInvert);
}

void UControlOptionsDlg::PushToModel(bool bApplyRuntimeToo)
{
    UStarshatterControlsSettings* Settings = GetControlsSettings();
    if (!Settings)
        return;

    FStarshatterControlsConfig C = Settings->GetControlsConfig();

    C.ControlModel = (EStarshatterControlModel)FMath::Clamp(ControlModel, 0, 2);

    C.JoystickIndex = FMath::Max(0, JoystickIndex);
    C.ThrottleAxis = FMath::Max(0, ThrottleAxis);
    C.RudderAxis = FMath::Max(0, RudderAxis);
    C.JoystickSensitivity = FMath::Clamp(JoystickSensitivity, 0, 10);

    C.MouseSensitivity = FMath::Clamp(MouseSensitivity, 0, 50);
    C.bMouseInvert = bMouseInvert;

    Settings->SetControlsConfig(C);
    Settings->Sanitize();
    Settings->Save();

    if (bApplyRuntimeToo)
        Settings->ApplyToRuntimeControls(this);

    bDirty = false;
}

// ---------------- Apply / Cancel ----------------

void UControlOptionsDlg::Apply()
{
    if (bClosed)
        return;

    PushToModel(true);
    bClosed = true;
}

void UControlOptionsDlg::Cancel()
{
    RefreshFromModel();
    bDirty = false;
    bClosed = true;
}

// ---------------- Runtime rows ----------------

void UControlOptionsDlg::BuildControlRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return;

    VBox->ClearChildren();

    auto Require = [&](UWidget* W, const TCHAR* Name) -> bool
        {
            if (!W)
            {
                UE_LOG(LogControlOptionsDlg, Error,
                    TEXT("[ControlOptionsDlg] Widget '%s' is NULL (BP name mismatch or not IsVariable)."), Name);
                return false;
            }
            return true;
        };

    const bool bOK =
        Require(ControlModelCombo, TEXT("ControlModelCombo")) &
        Require(JoystickIndexSlider, TEXT("JoystickIndexSlider")) &
        Require(ThrottleAxisSlider, TEXT("ThrottleAxisSlider")) &
        Require(RudderAxisSlider, TEXT("RudderAxisSlider")) &
        Require(JoystickSensitivitySlider, TEXT("JoystickSensitivitySlider")) &
        Require(MouseSensitivitySlider, TEXT("MouseSensitivitySlider")) &
        Require(MouseInvertCheck, TEXT("MouseInvertCheck"));

    if (!bOK)
    {
        UE_LOG(LogControlOptionsDlg, Error, TEXT("[ControlOptionsDlg] BuildControlRows aborted due to NULL controls."));
        return;
    }

    constexpr float W = 520.f;

    AddLabeledRow(TEXT("CONTROL MODEL"), ControlModelCombo, W);
    AddLabeledRow(TEXT("JOYSTICK INDEX"), JoystickIndexSlider, W);
    AddLabeledRow(TEXT("THROTTLE AXIS"), ThrottleAxisSlider, W);
    AddLabeledRow(TEXT("RUDDER AXIS"), RudderAxisSlider, W);
    AddLabeledRow(TEXT("JOYSTICK SENSITIVITY"), JoystickSensitivitySlider, W);
    AddLabeledRow(TEXT("MOUSE SENSITIVITY"), MouseSensitivitySlider, W);

    // Checkbox reads better narrower than a slider
    AddLabeledRow(TEXT("INVERT MOUSE"), MouseInvertCheck, 200.f);
}

void UControlOptionsDlg::BuildControlModelListIfNeeded()
{
    if (!ControlModelCombo)
        return;

    if (ControlModelCombo->GetOptionCount() > 0)
        return;

    ControlModelCombo->ClearOptions();

    // Must match your clamp 0..2 and enum order:
    ControlModelCombo->AddOption(TEXT("ARCADE"));
    ControlModelCombo->AddOption(TEXT("FLIGHTSIM"));
    ControlModelCombo->AddOption(TEXT("EXPERT"));

    ControlModelCombo->SetSelectedIndex(1);
}

// ---------------- Handlers ----------------

void UControlOptionsDlg::OnControlModelChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (ControlModelCombo)
    {
        ControlModel = ControlModelCombo->GetSelectedIndex();
        bDirty = true;
        bClosed = false;
    }
}

void UControlOptionsDlg::OnJoystickIndexChanged(float V)
{
    JoystickIndex = SliderToInt(V, 0, 8);
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnThrottleAxisChanged(float V)
{
    ThrottleAxis = SliderToInt(V, 0, 16);
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnRudderAxisChanged(float V)
{
    RudderAxis = SliderToInt(V, 0, 16);
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnJoystickSensitivityChanged(float V)
{
    JoystickSensitivity = SliderToInt(V, 0, 10);
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnMouseSensitivityChanged(float V)
{
    MouseSensitivity = SliderToInt(V, 0, 50);
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnMouseInvertChanged(bool bIsChecked)
{
    bMouseInvert = bIsChecked;
    bDirty = true;
    bClosed = false;
}

void UControlOptionsDlg::OnApplyClicked()
{
    if (OptionsManager)
        OptionsManager->ApplyOptions();
    else
        Apply();
}

void UControlOptionsDlg::OnCancelClicked()
{
    if (OptionsManager)
        OptionsManager->CancelOptions();
    else
        Cancel();
}

// Tabs -> route to OptionsScreen hub
void UControlOptionsDlg::OnAudioClicked() { if (OptionsManager) OptionsManager->ShowAudDlg(); }
void UControlOptionsDlg::OnVideoClicked() { if (OptionsManager) OptionsManager->ShowVidDlg(); }
void UControlOptionsDlg::OnControlsClicked() { if (OptionsManager) OptionsManager->ShowCtlDlg(); }
void UControlOptionsDlg::OnOptionsClicked() { if (OptionsManager) OptionsManager->ShowGameDlg(); }
void UControlOptionsDlg::OnModClicked() { if (OptionsManager) OptionsManager->ShowModDlg(); }

// ---------------- Slider helpers ----------------

int32 UControlOptionsDlg::SliderToInt(float Normalized, int32 MinV, int32 MaxV)
{
    const float Clamped = FMath::Clamp(Normalized, 0.0f, 1.0f);
    const float V = (float)MinV + (float)(MaxV - MinV) * Clamped;
    return FMath::RoundToInt(V);
}

float UControlOptionsDlg::IntToSlider(int32 V, int32 MinV, int32 MaxV)
{
    if (MaxV <= MinV) return 0.0f;
    const int32 Clamped = FMath::Clamp(V, MinV, MaxV);
    return (float)(Clamped - MinV) / (float)(MaxV - MinV);
}
