#include "LiveOptionsDlg.h"
#include "MissionUIStyle.h"
#include "StarshatterAudioSettings.h"
#include "StarshatterControlsSettings.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> ULiveOptionsDlg::RebuildWidget()
{
    SetIsFocusable(true);
    if (UGameInstance* Instance = GetGameInstance())
        if (auto* Save = Instance->GetSubsystem<UStarshatterSettingsSaveSubsystem>())
            Save->LoadOrCreate();

    const auto Settings = SNew(SVerticalBox);
    const auto AddSlider = [&Settings](const FText& Name, TAttribute<float> Value, FOnFloatValueChanged OnChanged)
    {
        Settings->AddSlot().AutoHeight().Padding(0, 8)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(Name).Font(MissionUIStyle::GetRowFont(17))]
            + SHorizontalBox::Slot().FillWidth(2).VAlign(VAlign_Center)
            [
                MissionUIStyle::MakeOptionsSliderTrack(
                    SNew(SSlider).Style(&MissionUIStyle::OptionsSliderStyle()).IndentHandle(false)
                        .Value(Value).OnValueChanged(OnChanged), Value)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0).VAlign(VAlign_Center)
            [
                SNew(SBox).WidthOverride(64)
                [
                    SNew(STextBlock).Justification(ETextJustify::Right).Font(MissionUIStyle::GetRowFont(16))
                    .Text_Lambda([Value]() { return FText::AsPercent(Value.Get()); })
                ]
            ]
        ];
    };

    auto* Audio = UStarshatterAudioSettings::Get();
    const auto AddVolume = [this, &AddSlider, Audio](const FText& Name,
        float (UStarshatterAudioSettings::*GetVolume)() const,
        void (UStarshatterAudioSettings::*SetVolume)(float))
    {
        AddSlider(Name, TAttribute<float>::CreateLambda([Audio, GetVolume]() { return (Audio->*GetVolume)(); }),
            FOnFloatValueChanged::CreateWeakLambda(this, [this, Audio, SetVolume](float Value)
            {
                (Audio->*SetVolume)(Value);
                Audio->ApplyToRuntimeAudio(GetGameInstance());
            }));
    };
    AddVolume(INVTEXT("Master volume"), &UStarshatterAudioSettings::GetMasterVolume, &UStarshatterAudioSettings::SetMasterVolume);
    AddVolume(INVTEXT("Music"), &UStarshatterAudioSettings::GetMusicVolume, &UStarshatterAudioSettings::SetMusicVolume);
    AddVolume(INVTEXT("Effects"), &UStarshatterAudioSettings::GetEffectsVolume, &UStarshatterAudioSettings::SetEffectsVolume);
    AddVolume(INVTEXT("Voice"), &UStarshatterAudioSettings::GetVoiceVolume, &UStarshatterAudioSettings::SetVoiceVolume);

    auto* Controls = UStarshatterControlsSettings::Get();
    AddSlider(INVTEXT("Mouse sensitivity"),
        TAttribute<float>::CreateLambda([Controls]() { return Controls->GetControlsConfig().MouseSensitivity / 50.f; }),
        FOnFloatValueChanged::CreateLambda([Controls](float Value) { Controls->SetMouseSensitivity(FMath::RoundToInt(Value * 50.f)); }));
    Settings->AddSlot().AutoHeight().Padding(0, 8)
    [
        SNew(SCheckBox)
        .IsChecked_Lambda([Controls]() { return Controls->GetControlsConfig().bMouseInvert ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
        .OnCheckStateChanged_Lambda([Controls](ECheckBoxState State) { Controls->SetMouseInvert(State == ECheckBoxState::Checked); })
        [SNew(STextBlock).Text(INVTEXT("Invert mouse pitch")).Font(MissionUIStyle::GetRowFont(17))]
    ];
    return SNew(SBox).Padding(24)[Settings];
}

void ULiveOptionsDlg::SaveSettings()
{
    UStarshatterAudioSettings::Get()->Save();
    UStarshatterControlsSettings::Get()->Save();
    if (UGameInstance* Instance = GetGameInstance())
        if (auto* Save = Instance->GetSubsystem<UStarshatterSettingsSaveSubsystem>())
        {
            Save->CaptureCurrentOptions();
            Save->MarkDirty();
            Save->Save();
        }
}

