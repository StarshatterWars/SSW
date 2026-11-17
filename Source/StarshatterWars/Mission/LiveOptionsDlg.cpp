#include "LiveOptionsDlg.h"
#include "MissionUIStyle.h"
#include "StarshatterAudioSettings.h"
#include "StarshatterControlsSettings.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "InputMappingContext.h"
#include "StarshatterInputBindings.h"
#include "StarshatterInputLabels.h"
#include "StarshatterKeyboardSettings.h"
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
    Settings->AddSlot().AutoHeight().Padding(0, 14, 0, 6)
    [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(17)).Text(INVTEXT("KEY BINDINGS"))];

    TArray<UInputMappingContext*> Contexts;
    for (const TCHAR* Path : {TEXT("/Game/Input/IMC_Game.IMC_Game"), TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD")})
        if (auto* Context = LoadObject<UInputMappingContext>(nullptr, Path)) Contexts.AddUnique(Context);
    TSoftObjectPtr<UInputMappingContext> Model;
    switch (Controls->GetControlsConfig().ControlModel)
    {
        case EStarshatterControlModel::Arcade: Model = Controls->GetArcadeContext(); break;
        case EStarshatterControlModel::FlightSim: Model = Controls->GetFlightSimContext(); break;
        case EStarshatterControlModel::Hybrid: Model = Controls->GetHybridContext(); break;
        default: break;
    }
    if (auto* Context = Model.LoadSynchronous()) Contexts.AddUnique(Context);
    if (auto* Context = StarshatterInputBindings::SupplementalContext(this)) Contexts.AddUnique(Context);

    TArray<TPair<FString, FString>> Bindings;
    const auto& Config = UStarshatterKeyboardSettings::Get()->GetKeyboardConfig();
    TSet<FString> Seen;
    TMap<FKey, TArray<FString>> KeyFunctions;
    KeyFunctions.FindOrAdd(EKeys::Escape).Add(TEXT("Mission pause / exit"));
    Bindings.Emplace(TEXT("Mission pause / exit"), EKeys::Escape.GetDisplayName().ToString());
    for (const auto* Context : Contexts)
        for (auto Mapping : Context->GetMappings())
        {
            if (!Mapping.Action || Mapping.Key.IsGamepadKey() || Mapping.Key.IsMouseButton() || Mapping.Key.IsAxis1D() || Mapping.Key.IsAxis2D()) continue;
            const FName Action = Mapping.Action->GetFName();
            if (Action == TEXT("IA_Quit") || Action == TEXT("IA_ViewCockpit") || Action == TEXT("IA_ViewVirtualCockpit") ||
                Action == TEXT("IA_ViewChase") || Action == TEXT("IA_ViewOrbit") || Action == TEXT("IA_MouseZoom") ||
                Action == TEXT("IA_GroundMode") || Action == TEXT("IA_HudMode")) continue;
            const FString Id = StarshatterInputBindings::Id(Mapping);
            if (Seen.Contains(Id)) continue;
            Seen.Add(Id);
            if (const FKey* Saved = Config.EnhancedBindings.Find(Id)) Mapping.Key = *Saved;
            if (Mapping.Key.IsGamepadKey() || Mapping.Key.IsMouseButton()) continue;

            FString Key = Mapping.Key.IsValid()
                ? UStarshatterInputLabels::GetBindingKeyLabel(Mapping).ToString() : TEXT("Unbound");
            if (Mapping.Key.IsValid() && (Action == TEXT("SSW_SensorRangeUp") || Action == TEXT("SSW_SensorRangeDown")))
                Key = TEXT("Shift + ") + Key;
            const FString Function = UStarshatterInputLabels::GetActionLabel(Mapping.Action).ToString();
            if (Mapping.Key.IsValid())
                KeyFunctions.FindOrAdd(Mapping.Key).AddUnique(Function);
            Bindings.Emplace(Function, MoveTemp(Key));
        }
    const auto KeyboardZoom = MakeShared<float>(0.0f);
    Settings->AddSlot().AutoHeight().Padding(0, 4, 0, 8)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(INVTEXT("Keyboard zoom"))]
        + SHorizontalBox::Slot().FillWidth(1.0f)
        [SNew(SSlider).Value(0.0f).OnValueChanged_Lambda([KeyboardZoom](float Value) { *KeyboardZoom = Value; })]
    ];
    auto Keyboard = SNew(SVerticalBox);
    const TCHAR* KeyRows[] =
    {
        TEXT("Escape F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12"),
        TEXT("Tilde One Two Three Four Five Six Seven Eight Nine Zero Hyphen Equals BackSpace"),
        TEXT("Tab Q W E R T Y U I O P LeftBracket RightBracket Backslash"),
        TEXT("CapsLock A S D F G H J K L Semicolon Apostrophe Enter"),
        TEXT("LeftShift Z X C V B N M Comma Period Slash RightShift"),
        TEXT("LeftControl LeftAlt SpaceBar RightAlt RightControl Left Down Up Right"),
        TEXT("Insert Home PageUp Delete End PageDown NumPadZero NumPadOne NumPadTwo NumPadThree"),
        TEXT("NumPadFour NumPadFive NumPadSix NumPadSeven NumPadEight NumPadNine")
    };
    for (const TCHAR* KeyRow : KeyRows)
    {
        TArray<FString> Names;
        FString(KeyRow).ParseIntoArrayWS(Names);
        auto Row = SNew(SHorizontalBox);
        for (const FString& Name : Names)
        {
            const FKey Key(*Name);
            const auto* Functions = KeyFunctions.Find(Key);
            const FString Actions = Functions ? FString::Join(*Functions, TEXT("\n")) : FString();
            const FText KeyLabel = Key.GetDisplayName(false);
            const FText Tooltip = FText::FromString(Actions.IsEmpty() ? KeyLabel.ToString() : KeyLabel.ToString() + TEXT("\n") + Actions);
            Row->AddSlot().AutoWidth().Padding(2)
            [
                SNew(SBox).WidthOverride_Lambda([KeyboardZoom]() { return FMath::Lerp(54.f, 132.f, *KeyboardZoom); })
                [
                    SNew(SBorder).Padding(4).ToolTipText(Tooltip)
                    .BorderBackgroundColor(Functions ? FLinearColor(0.16f, 0.32f, 0.40f) : FLinearColor(0.12f, 0.15f, 0.19f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(KeyLabel)]
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(STextBlock).Font(MissionUIStyle::GetRowFont(10))
                            .Text(FText::FromString(Actions)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                        ]
                    ]
                ]
            ];
        }
        Keyboard->AddSlot().AutoHeight()[Row];
    }
    Settings->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
    [SNew(SScrollBox).Orientation(Orient_Horizontal) + SScrollBox::Slot()[Keyboard]];
    Bindings.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });
    for (const auto& Binding : Bindings)
        Settings->AddSlot().AutoHeight().Padding(0, 3)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(2)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(FText::FromString(Binding.Key))]
            + SHorizontalBox::Slot().FillWidth(1)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(FText::FromString(Binding.Value))]
        ];
    return SNew(SBox).Padding(24)[SNew(SScrollBox) + SScrollBox::Slot()[Settings]];
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

