#include "LiveOptionsDlg.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "PlayerCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Widgets/Input/SButton.h"
#include "StarshatterAudioSettings.h"
#include "StarshatterControlsSettings.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "InputMappingContext.h"
#include "StarshatterInputBindings.h"
#include "StarshatterInputLabels.h"
#include "StarshatterKeyboardSettings.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    class SKeyboardScrollBox : public SScrollBox
    {
    public:
        FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            if (Event.GetEffectingButton() != EKeys::LeftMouseButton || GetScrollOffsetOfEnd() <= 0.f)
                return SScrollBox::OnMouseButtonDown(Geometry, Event);

            DragOrigin = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
            DragStartOffset = GetScrollOffset();
            return FReply::Handled().CaptureMouse(SharedThis(this));
        }

        FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            if (!HasMouseCapture() || !Event.IsMouseButtonDown(EKeys::LeftMouseButton))
                return SScrollBox::OnMouseMove(Geometry, Event);

            const double Position = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X;
            SetScrollOffset(FMath::Clamp(static_cast<float>(DragStartOffset + DragOrigin - Position),
                0.f, GetScrollOffsetOfEnd()));
            return FReply::Handled();
        }

        FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override
        {
            if (Event.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
                return FReply::Handled().ReleaseMouseCapture();
            return SScrollBox::OnMouseButtonUp(Geometry, Event);
        }

    private:
        double DragOrigin = 0.0;
        float DragStartOffset = 0.f;
    };
}

TSharedRef<SWidget> ULiveOptionsDlg::RebuildWidget()
{
    SetIsFocusable(true);
    if (UGameInstance* Instance = GetGameInstance())
        if (auto* Save = Instance->GetSubsystem<UStarshatterSettingsSaveSubsystem>())
            Save->LoadOrCreate();

    const auto AudioPage = SNew(SVerticalBox);
    const auto VideoPage = SNew(SVerticalBox);
    const auto KeyboardPage = SNew(SVerticalBox);
    auto Settings = AudioPage;
    const auto AddSection = [&Settings](EShipSystemIcon Icon, FText Label)
    {
        Settings->AddSlot().AutoHeight().Padding(0, 14, 0, 6)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
            [SNew(SShipSystemIcon).Icon(Icon)]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [SNew(STextBlock).Font(MissionUIStyle::GetHeaderFont(15)).Text(Label)]
        ];
    };
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

    AddSection(EShipSystemIcon::Audio, INVTEXT("AUDIO"));
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

    Settings = KeyboardPage;
    AddSection(EShipSystemIcon::Options, INVTEXT("FLIGHT CONTROLS"));
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
    Settings = VideoPage;
    AddSection(EShipSystemIcon::Display, INVTEXT("VIDEO AND HUD"));
    if (auto* Video = GEngine ? GEngine->GetGameUserSettings() : nullptr)
    {
        Settings->AddSlot().AutoHeight().Padding(0, 8)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([Video]() { return Video->IsVSyncEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
            .OnCheckStateChanged_Lambda([Video](ECheckBoxState State)
            {
                Video->SetVSyncEnabled(State == ECheckBoxState::Checked);
                Video->ApplyNonResolutionSettings();
                Video->SaveSettings();
            })
            [SNew(STextBlock).Text(INVTEXT("Vertical sync")).Font(MissionUIStyle::GetRowFont(17))]
        ];
    }

    Settings->AddSlot().AutoHeight().Padding(0, 8)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
        [SNew(STextBlock).Text(INVTEXT("HUD palette")).Font(MissionUIStyle::GetRowFont(17))]
        + SHorizontalBox::Slot().AutoWidth()
        [
            SNew(SButton).ToolTipText(INVTEXT("Cycle HUD colors"))
            .OnClicked_Lambda([]()
            {
                const int Color = (HUDView::DefaultColorSet() + 1) % 4;
                HUDView::SetDefaultColorSet(Color);
                if (auto* Hud = HUDView::GetInstance()) Hud->SetHUDColorSet(Color);
                if (auto* Pilot = PlayerCharacter::GetCurrentPlayer()) Pilot->SetHUDColor(Color);
                return FReply::Handled();
            })
            [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Refresh)]
        ]
    ];
    Settings->AddSlot().AutoHeight().Padding(0, 8)
    [
        SNew(SCheckBox)
        .IsChecked_Lambda([]() { return HUDView::GetGunsight() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
        .OnCheckStateChanged_Lambda([](ECheckBoxState State)
        {
            const bool bEnabled = State == ECheckBoxState::Checked;
            HUDView::SetGunsight(bEnabled);
            if (auto* Pilot = PlayerCharacter::GetCurrentPlayer()) Pilot->SetGunsight(bEnabled);
        })
        [SNew(STextBlock).Text(INVTEXT("Gunsight")).Font(MissionUIStyle::GetRowFont(17))]
    ];
    Settings = KeyboardPage;
    AddSection(EShipSystemIcon::Keyboard, INVTEXT("KEY BINDINGS"));

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
            const bool bShift = Action == TEXT("SSW_SensorRangeUp") || Action == TEXT("SSW_SensorRangeDown");
            if (Mapping.Key.IsValid() && bShift) Key = TEXT("Shift + ") + Key;
            const FString Function = UStarshatterInputLabels::GetActionLabel(Mapping.Action).ToString();
            if (Mapping.Key.IsValid())
                KeyFunctions.FindOrAdd(Mapping.Key).AddUnique(bShift ? TEXT("Shift + ") + Function : Function);
            Bindings.Emplace(Function, MoveTemp(Key));
        }
    const auto KeyboardZoom = MakeShared<float>(0.48f);
    Settings->AddSlot().AutoHeight().Padding(0, 4, 0, 8)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
        [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(14)).Text(INVTEXT("Keyboard zoom"))]
        + SHorizontalBox::Slot().FillWidth(1.0f)
        [SNew(SSlider).Value(*KeyboardZoom).OnValueChanged_Lambda([KeyboardZoom](float Value) { *KeyboardZoom = Value; })]
    ];
    auto Keyboard = SNew(SConstraintCanvas);
    // US Windows layout without the numeric keypad. Positions and widths are measured in key units.
    const auto AddKey = [&Keyboard, KeyboardZoom, &KeyFunctions](const FString& Name, float X, float Y, float W, float H)
    {
        const FKey Key(*Name);
        const auto* Functions = KeyFunctions.Find(Key);
        const FString Actions = Functions ? FString::Join(*Functions, TEXT("\n")) : FString();
        static const TMap<FString, FString> ShortLabels = {
            {TEXT("LeftCommand"), TEXT("Win")}, {TEXT("RightCommand"), TEXT("Win")}, {TEXT("Menu"), TEXT("Menu")},
            {TEXT("LeftControl"), TEXT("Ctrl")}, {TEXT("RightControl"), TEXT("Ctrl")},
            {TEXT("LeftShift"), TEXT("Shift")}, {TEXT("RightShift"), TEXT("Shift")},
            {TEXT("LeftAlt"), TEXT("Alt")}, {TEXT("RightAlt"), TEXT("Alt")},
            {TEXT("PrintScreen"), TEXT("PrtSc")}, {TEXT("ScrollLock"), TEXT("ScrLk")},
            {TEXT("PageUp"), TEXT("PgUp")}, {TEXT("PageDown"), TEXT("PgDn")}, {TEXT("BackSpace"), TEXT("Back")},
            {TEXT("SpaceBar"), TEXT("Space")}, {TEXT("NumPadEnter"), TEXT("Enter")}, {TEXT("NumLock"), TEXT("Num")},
            {TEXT("Divide"), TEXT("/")}, {TEXT("Multiply"), TEXT("*")}, {TEXT("Subtract"), TEXT("-")},
            {TEXT("Add"), TEXT("+")}, {TEXT("Decimal"), TEXT(".")}
        };
        const auto* ShortLabel = ShortLabels.Find(Name);
        const FText KeyLabel = ShortLabel ? FText::FromString(*ShortLabel) :
            Key.IsValid() ? Key.GetDisplayName(false) : FText::FromString(Name);
        const FText Tooltip = FText::FromString(Actions.IsEmpty() ? KeyLabel.ToString() : KeyLabel.ToString() + TEXT("\n") + Actions);
        Keyboard->AddSlot().Offset(TAttribute<FMargin>::CreateLambda([KeyboardZoom, X, Y, W, H]()
        {
            const float Unit = FMath::Lerp(36.f, 76.f, *KeyboardZoom);
            return FMargin(X * Unit + 2, Y * Unit + 2, W * Unit - 4, H * Unit - 4);
        }))
        [
            SNew(SBorder).Padding(FMargin(3, 2)).ToolTipText(Tooltip)
            .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(Functions ? FLinearColor(.12f, .28f, .38f) : FLinearColor(.09f, .11f, .14f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(11)).Text(KeyLabel)
                    .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Font(MissionUIStyle::GetRowFont(9))
                    .Text(FText::FromString(Actions)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                ]
            ]
        ];
    };
    const auto AddRow = [&AddKey](float X, float Y, const TCHAR* Spec)
    {
        TArray<FString> Keys;
        FString(Spec).ParseIntoArrayWS(Keys);
        for (const FString& Token : Keys)
        {
            FString Name = Token;
            FString Width;
            float Size = 1;
            if (Token.Split(TEXT(":"), &Name, &Width)) Size = FCString::Atof(*Width);
            if (Name != TEXT("_")) AddKey(Name, X, Y, Size, 1);
            X += Size;
        }
    };
    AddRow(0, 0, TEXT("Escape _:1 F1 F2 F3 F4 _:.5 F5 F6 F7 F8 _:.5 F9 F10 F11 F12"));
    AddRow(0, 1.4, TEXT("Tilde One Two Three Four Five Six Seven Eight Nine Zero Hyphen Equals BackSpace:2"));
    AddRow(0, 2.4, TEXT("Tab:1.5 Q W E R T Y U I O P LeftBracket RightBracket Backslash:1.5"));
    AddRow(0, 3.4, TEXT("CapsLock:1.75 A S D F G H J K L Semicolon Apostrophe Enter:2.25"));
    AddRow(0, 4.4, TEXT("LeftShift:2.25 Z X C V B N M Comma Period Slash RightShift:2.75"));
    AddRow(0, 5.4, TEXT("LeftControl:1.25 LeftCommand:1.25 LeftAlt:1.25 SpaceBar:6.25 RightAlt:1.25 RightCommand:1.25 Menu:1.25 RightControl:1.25"));
    AddRow(15.35, 0, TEXT("PrintScreen ScrollLock Pause"));
    AddRow(15.35, 1.4, TEXT("Insert Home PageUp"));
    AddRow(15.35, 2.4, TEXT("Delete End PageDown"));
    AddRow(16.35, 4.4, TEXT("Up"));
    AddRow(15.35, 5.4, TEXT("Left Down Right"));
    Settings->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
    [
        SNew(SKeyboardScrollBox).Orientation(Orient_Horizontal)
            .Clipping(EWidgetClipping::ClipToBounds)
            .ToolTipText(INVTEXT("Drag to scroll the keyboard"))
        + SScrollBox::Slot()
        [
            SNew(SBox)
            .WidthOverride_Lambda([KeyboardZoom]() { return 18.35f * FMath::Lerp(36.f, 76.f, *KeyboardZoom); })
            .HeightOverride_Lambda([KeyboardZoom]() { return 6.4f * FMath::Lerp(36.f, 76.f, *KeyboardZoom); })
            [Keyboard]
        ]
    ];
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
    const auto Pages = SNew(SWidgetSwitcher)
        + SWidgetSwitcher::Slot()[SNew(SScrollBox) + SScrollBox::Slot()[AudioPage]]
        + SWidgetSwitcher::Slot()[SNew(SScrollBox) + SScrollBox::Slot()[VideoPage]]
        + SWidgetSwitcher::Slot()[SNew(SScrollBox) + SScrollBox::Slot()[KeyboardPage]];
    const auto Tabs = SNew(SHorizontalBox);
    const TArray<EShipSystemIcon> Icons = {EShipSystemIcon::Audio, EShipSystemIcon::Display, EShipSystemIcon::Keyboard};
    const TArray<FText> Labels = {INVTEXT("Audio"), INVTEXT("Video and HUD"), INVTEXT("Keyboard and flight controls")};
    for (int32 Index = 0; Index < Icons.Num(); ++Index)
        Tabs->AddSlot().AutoWidth().Padding(0,0,8,0)
        [SNew(SButton).ContentPadding(FMargin(14,8)).ToolTipText(Labels[Index])
            .ButtonColorAndOpacity_Lambda([Pages,Index]() { return Pages->GetActiveWidgetIndex() == Index ? FLinearColor(.12f,.42f,.55f) : FLinearColor(.12f,.18f,.23f); })
            .OnClicked_Lambda([Pages,Index]() { Pages->SetActiveWidgetIndex(Index); return FReply::Handled(); })
            [SNew(SShipSystemIcon).Icon(Icons[Index])]];
    return SNew(SBox).Padding(24)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[Tabs]
            + SVerticalBox::Slot().FillHeight(1)[Pages]];
}

void ULiveOptionsDlg::SaveSettings()
{
    PlayerCharacter::Save();
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

