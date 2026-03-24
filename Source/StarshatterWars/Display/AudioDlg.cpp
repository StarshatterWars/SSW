/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           AudioDlg.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UAudioDlg

    Notes:
    - Delegates are bound once via AddUniqueDynamic (no RemoveAll needed).
    - OptionsScreen is the router; this page forwards if OptionsManager exists.
    - Enter/Escape routing is handled via UBaseScreen (ApplyButton/CancelButton)
      plus HandleAccept/HandleCancel overrides.

=============================================================================*/

#include "AudioDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"

// UMG layout helpers
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"

// Model
#include "StarshatterAudioSettings.h"

// Save system
#include "StarshatterSettingsSaveSubsystem.h"
#include "StarshatterSettingsSaveGame.h"

// Router (cpp-only include to avoid header coupling)
#include "OptionsScreen.h"

// UE
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogAudioDlg, Log, All);

UAudioDlg::UAudioDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UAudioDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // If UBaseScreen supports these pointers, set them once:
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();
}

void UAudioDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    UE_LOG(LogAudioDlg, Warning, TEXT("[AudioDlg] PreConstruct RootCanvas=%s WidgetTree=%s"),
        RootCanvas ? TEXT("VALID") : TEXT("NULL"),
        WidgetTree ? TEXT("VALID") : TEXT("NULL"));
    EnsureAutoVerticalBox();
    AutoVBox->ClearChildren();
}

void UAudioDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegates();

    // Ensure VBox exists and is visible:
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
    {
        UE_LOG(LogAudioDlg, Error, TEXT("[AudioDlg] BuildRows FAILED: AutoVBox is NULL"));
        return;
    }

    VBox->SetVisibility(ESlateVisibility::Visible);
    BuildQualityListIfNeeded();
    // IMPORTANT: build rows AFTER widgets are bound
    BuildAudioRows();

    // Sanity log
    UE_LOG(LogAudioDlg, Warning, TEXT("[AudioDlg] AutoVBox children after BuildAudioRows: %d"),
        VBox->GetChildrenCount());

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

void UAudioDlg::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Apply/Cancel
    if (ApplyBtn)  ApplyBtn->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnApplyClicked);
    if (CancelBtn) CancelBtn->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnCancelClicked);

    // Optional local tabs (only if present in the page WBP)
    if (AudTabButton) AudTabButton->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnAudioClicked);
    if (VidTabButton) VidTabButton->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnVideoClicked);
    if (CtlTabButton) CtlTabButton->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnControlsClicked);
    if (OptTabButton) OptTabButton->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnOptionsClicked);
    if (ModTabButton) ModTabButton->OnClicked.AddUniqueDynamic(this, &UAudioDlg::OnModClicked);

    // Sliders
    if (MasterSlider)  MasterSlider->OnValueChanged.AddUniqueDynamic(this, &UAudioDlg::OnMasterVolumeChanged);
    if (MusicSlider)   MusicSlider->OnValueChanged.AddUniqueDynamic(this, &UAudioDlg::OnMusicVolumeChanged);
    if (EffectsSlider) EffectsSlider->OnValueChanged.AddUniqueDynamic(this, &UAudioDlg::OnEffectsVolumeChanged);
    if (VoiceSlider)   VoiceSlider->OnValueChanged.AddUniqueDynamic(this, &UAudioDlg::OnVoiceVolumeChanged);

    // Combo
    if (QualityCombo)
        QualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UAudioDlg::OnSoundQualityChanged);

    bDelegatesBound = true;
}

void UAudioDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame((double)InDeltaTime);
}

void UAudioDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling required.
}

void UAudioDlg::BindFormWidgets() {}
FString UAudioDlg::GetLegacyFormText() const { return FString(); }

void UAudioDlg::HandleAccept() { OnApplyClicked(); }
void UAudioDlg::HandleCancel() { OnCancelClicked(); }

void UAudioDlg::Show()
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

UStarshatterAudioSettings* UAudioDlg::GetAudioSettings() const
{
    return GetMutableDefault<UStarshatterAudioSettings>();
}

UStarshatterSettingsSaveSubsystem* UAudioDlg::GetSettingsSaveSubsystem() const
{
    if (UGameInstance* GI = GetGameInstance())
        return GI->GetSubsystem<UStarshatterSettingsSaveSubsystem>();
    return nullptr;
}

void UAudioDlg::RefreshFromModel()
{
    UStarshatterAudioSettings* S = GetAudioSettings();
    if (!S)
        return;

    S->ReloadConfig();
    S->Sanitize();

    MasterVolume = S->GetMasterVolume();
    MusicVolume = S->GetMusicVolume();
    EffectsVolume = S->GetEffectsVolume();
    VoiceVolume = S->GetVoiceVolume();
    SoundQuality = S->GetSoundQuality();

    if (MasterSlider)  MasterSlider->SetValue(MasterVolume);
    if (MusicSlider)   MusicSlider->SetValue(MusicVolume);
    if (EffectsSlider) EffectsSlider->SetValue(EffectsVolume);
    if (VoiceSlider)   VoiceSlider->SetValue(VoiceVolume);
    if (QualityCombo)  QualityCombo->SetSelectedIndex(SoundQuality);
}

void UAudioDlg::PushToModel(bool bApplyRuntimeToo)
{
    UStarshatterAudioSettings* S = GetAudioSettings();
    if (!S)
        return;

    S->SetMasterVolume(MasterVolume);
    S->SetMusicVolume(MusicVolume);
    S->SetEffectsVolume(EffectsVolume);
    S->SetVoiceVolume(VoiceVolume);
    S->SetSoundQuality(SoundQuality);

    S->Sanitize();
    S->Save();

    SaveAudioToUnifiedSettings(S);

    if (bApplyRuntimeToo)
        S->ApplyToRuntimeAudio(this);

    bDirty = false;
}

void UAudioDlg::SaveAudioToUnifiedSettings(UStarshatterAudioSettings* Settings)
{
    if (!Settings)
        return;

    UStarshatterSettingsSaveSubsystem* SaveSS = GetSettingsSaveSubsystem();
    if (!SaveSS)
        return;

    SaveSS->LoadOrCreate();

    UStarshatterSettingsSaveGame* SG = SaveSS->GetSettings();
    if (!SG)
        return;

    SG->Audio.MasterVolume = Settings->GetMasterVolume();
    SG->Audio.MusicVolume = Settings->GetMusicVolume();
    SG->Audio.EffectsVolume = Settings->GetEffectsVolume();
    SG->Audio.VoiceVolume = Settings->GetVoiceVolume();
    SG->Audio.SoundQuality = Settings->GetSoundQuality();

    SG->Sanitize();
    SaveSS->MarkDirty();
    SaveSS->Save();
}

// ---------------- Apply / Cancel ----------------

void UAudioDlg::Apply()
{
    if (bClosed)
        return;

    // Only push if something actually changed
    PushToModel(true);
    bClosed = true;
}

void UAudioDlg::Cancel()
{
    RefreshFromModel();
    bDirty = false;
    bClosed = true;
}

// ---------------- Handlers ----------------

void UAudioDlg::OnMasterVolumeChanged(float V) { MasterVolume = V; bDirty = true; bClosed = false; }
void UAudioDlg::OnMusicVolumeChanged(float V) { MusicVolume = V; bDirty = true; bClosed = false; }
void UAudioDlg::OnEffectsVolumeChanged(float V) { EffectsVolume = V; bDirty = true; bClosed = false; }
void UAudioDlg::OnVoiceVolumeChanged(float V) { VoiceVolume = V; bDirty = true; bClosed = false; }

void UAudioDlg::OnSoundQualityChanged(FString /*SelectedItem*/, ESelectInfo::Type /*SelectionType*/)
{
    if (QualityCombo)
    {
        SoundQuality = QualityCombo->GetSelectedIndex();
        bDirty = true;
        bClosed = false;
    }
}

void UAudioDlg::OnApplyClicked()
{
    if (OptionsManager)
        OptionsManager->ApplyOptions();
    else
        Apply();
}

void UAudioDlg::OnCancelClicked()
{
    if (OptionsManager)
        OptionsManager->ApplyOptions();
    else 
        Cancel();
}

void UAudioDlg::BuildAudioRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return;

    // Clear any previous runtime rows (optional, but avoids duplicates if Construct fires again)
    VBox->ClearChildren();

    auto Require = [&](UWidget* W, const TCHAR* Name) -> bool
        {
            if (!W)
            {
                UE_LOG(LogAudioDlg, Error, TEXT("[AudioDlg] Widget '%s' is NULL (BP name mismatch or not IsVariable)."), Name);
                return false;
            }
            return true;
        };

    // If any control is null, your rows will be empty/no-op:
    const bool bOK =
        Require(MasterSlider, TEXT("MasterSlider")) &
        Require(MusicSlider, TEXT("MusicSlider")) &
        Require(EffectsSlider, TEXT("EffectsSlider")) &
        Require(VoiceSlider, TEXT("VoiceSlider")) &
        Require(QualityCombo, TEXT("QualityCombo"));

    if (!bOK)
    {
        UE_LOG(LogAudioDlg, Error, TEXT("[AudioDlg] BuildAudioRows aborted due to NULL controls."));
        return;
    }

    AddLabeledRow(TEXT("MASTER VOLUME"), MasterSlider, 520.f);
    AddLabeledRow(TEXT("MUSIC VOLUME"), MusicSlider, 520.f);
    AddLabeledRow(TEXT("EFFECTS VOLUME"), EffectsSlider, 520.f);
    AddLabeledRow(TEXT("VOICE VOLUME"), VoiceSlider, 520.f);
    AddLabeledRow(TEXT("SOUND QUALITY"), QualityCombo, 520.f);
}

void UAudioDlg::BuildQualityListIfNeeded()
{
    if (!QualityCombo)
        return;

    // If it already has items (BP or previous construct), don’t duplicate:
    if (QualityCombo->GetOptionCount() > 0)
        return;

    QualityCombo->ClearOptions();

    // Keep it simple (blue/grey UI is fine; labels can be whatever you want):
    QualityCombo->AddOption(TEXT("LOW"));
    QualityCombo->AddOption(TEXT("MEDIUM"));
    QualityCombo->AddOption(TEXT("HIGH"));

    // Optional: default selection so it shows something even before RefreshFromModel:
    QualityCombo->SetSelectedIndex(1);
}

// Optional local tabs -> route to OptionsScreen hub
void UAudioDlg::OnAudioClicked() { if (OptionsManager) OptionsManager->ShowAudDlg(); }
void UAudioDlg::OnVideoClicked() { if (OptionsManager) OptionsManager->ShowVidDlg(); }
void UAudioDlg::OnControlsClicked() { if (OptionsManager) OptionsManager->ShowCtlDlg(); }
void UAudioDlg::OnOptionsClicked() { if (OptionsManager) OptionsManager->ShowGameDlg(); }
void UAudioDlg::OnModClicked() { if (OptionsManager) OptionsManager->ShowModDlg(); }
