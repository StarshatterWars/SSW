/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           KeyDlg.cpp
    AUTHOR:         Carlos Bott

    NOTES
    =====
    - UE-only key binding list with scrollbox
    - Shows defaults (legacy defmap) as fallback display
    - Pending remaps + clears override display until Apply
    - Value/key text uses (0.5,0.5,0.5,1.0)
=============================================================================*/

#include "KeyDlg.h"

// UMG
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "GameStructs.h"
#include "GameStructs_InputDefaults.h"

// Router
#include "OptionsScreen.h"

// Model + runtime
#include "StarshatterKeyboardSettings.h"
#include "StarshatterKeyboardSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogKeyDlg, Log, All);

// ------------------------------------------------------------
// Legacy defaults -> UE keys (fallback display)
// ------------------------------------------------------------

static bool GetDefaultKeyForAction(
    EStarshatterInputAction Action,
    FKey& OutKey,
    bool& bOutShift,
    bool& bOutCtrl,
    bool& bOutAlt
)
{
    bOutShift = false;
    bOutCtrl = false;
    bOutAlt = false;
    OutKey = FKey();

    // IMPORTANT:
    // This mapping MUST match your EStarshatterInputAction enumerators.
    // If your enum uses different names, update the case labels only.

    switch (Action)
    {
        // --------------------------------------------------------
        // Flight controls
        // --------------------------------------------------------
    case EStarshatterInputAction::PitchUp:        OutKey = EKeys::Down;          return true; // VK_DOWN
    case EStarshatterInputAction::PitchDown:      OutKey = EKeys::Up;            return true; // VK_UP
    case EStarshatterInputAction::YawLeft:        OutKey = EKeys::Left;          return true; // VK_LEFT
    case EStarshatterInputAction::YawRight:       OutKey = EKeys::Right;         return true; // VK_RIGHT
    case EStarshatterInputAction::RollLeft:       OutKey = EKeys::NumPadSeven;   return true; // VK_NUMPAD7
    case EStarshatterInputAction::RollRight:      OutKey = EKeys::NumPadNine;    return true; // VK_NUMPAD9

        // KEY_PLUS_X  '.'  (190)
    case EStarshatterInputAction::PlusX:          OutKey = EKeys::Period;        return true;
        // KEY_MINUS_X ','  (188)
    case EStarshatterInputAction::MinusX:         OutKey = EKeys::Comma;         return true;
    case EStarshatterInputAction::PlusY:          OutKey = EKeys::Home;          return true; // VK_HOME
    case EStarshatterInputAction::MinusY:         OutKey = EKeys::End;           return true; // VK_END
    case EStarshatterInputAction::PlusZ:          OutKey = EKeys::PageUp;        return true; // VK_PRIOR
    case EStarshatterInputAction::MinusZ:         OutKey = EKeys::PageDown;      return true; // VK_NEXT

        // Actions
    case EStarshatterInputAction::Action0:        OutKey = EKeys::LeftControl;   return true; // VK_CONTROL
    case EStarshatterInputAction::Action1:        OutKey = EKeys::SpaceBar;      return true; // VK_SPACE

        // Throttle
    case EStarshatterInputAction::ThrottleUp:     OutKey = EKeys::A;             return true; // 'A'
    case EStarshatterInputAction::ThrottleDown:   OutKey = EKeys::Z;             return true; // 'Z'
    case EStarshatterInputAction::ThrottleFull:   OutKey = EKeys::A; bOutShift = true; return true; // 'A' + SHIFT
    case EStarshatterInputAction::ThrottleZero:   OutKey = EKeys::Z; bOutShift = true; return true; // 'Z' + SHIFT
    case EStarshatterInputAction::Augmenter:      OutKey = EKeys::Tab;           return true; // VK_TAB
    case EStarshatterInputAction::FlcsModeAuto:   OutKey = EKeys::M;             return true; // 'M'
    case EStarshatterInputAction::CommandMode:    OutKey = EKeys::M; bOutShift = true; return true; // 'M' + SHIFT

        // Weapons/targeting
    case EStarshatterInputAction::CyclePrimary:   OutKey = EKeys::BackSpace; bOutShift = true; return true; // VK_BACK + SHIFT
    case EStarshatterInputAction::CycleSecondary: OutKey = EKeys::BackSpace;      return true; // VK_BACK
    case EStarshatterInputAction::FirePrimary:    OutKey = EKeys::LeftControl;    return true;
    case EStarshatterInputAction::FireSecondary:  OutKey = EKeys::SpaceBar;       return true;

    case EStarshatterInputAction::LockTarget:     OutKey = EKeys::T;             return true; // 'T'
    case EStarshatterInputAction::LockThreat:     OutKey = EKeys::T; bOutShift = true; return true; // 'T' + SHIFT
    case EStarshatterInputAction::LockClosestShip:OutKey = EKeys::U;             return true; // 'U'
    case EStarshatterInputAction::LockClosestThreat:OutKey = EKeys::U; bOutShift = true; return true; // 'U' + SHIFT
    case EStarshatterInputAction::LockHostileShip:OutKey = EKeys::Y;             return true; // 'Y'
    case EStarshatterInputAction::LockHostileThreat:OutKey = EKeys::Y; bOutShift = true; return true; // 'Y' + SHIFT

        // Subtarget cycle (186 = ';' on VK_OEM_1, but your legacy used 186 directly)
        // UE key naming for OEM may differ by layout; keep as Semicolon as best effort:
    case EStarshatterInputAction::CycleSubtarget: OutKey = EKeys::Semicolon;     return true;
    case EStarshatterInputAction::PrevSubtarget:  OutKey = EKeys::Semicolon; bOutShift = true; return true;

        // Decoy / gear / lights
    case EStarshatterInputAction::Decoy:          OutKey = EKeys::D;             return true; // 'D'
    case EStarshatterInputAction::GearToggle:     OutKey = EKeys::G;             return true; // 'G'
    case EStarshatterInputAction::NavlightToggle: OutKey = EKeys::L;             return true; // 'L'
    case EStarshatterInputAction::AutoNav:        OutKey = EKeys::N; bOutShift = true; return true; // 'N' + SHIFT
    case EStarshatterInputAction::DropOrbit:      OutKey = EKeys::O;             return true; // 'O'

        // Shields
    case EStarshatterInputAction::ShieldsUp:      OutKey = EKeys::S;             return true; // 'S'
    case EStarshatterInputAction::ShieldsDown:    OutKey = EKeys::X;             return true; // 'X'
    case EStarshatterInputAction::ShieldsFull:    OutKey = EKeys::S; bOutShift = true; return true; // 'S' + SHIFT
    case EStarshatterInputAction::ShieldsZero:    OutKey = EKeys::X; bOutShift = true; return true; // 'X' + SHIFT

        // Sensors
    case EStarshatterInputAction::SensorMode:         OutKey = EKeys::F5;  return true;
    case EStarshatterInputAction::SensorGroundMode:   OutKey = EKeys::F5;  bOutShift = true; return true;
    case EStarshatterInputAction::LaunchProbe:        OutKey = EKeys::F6;  return true;
    case EStarshatterInputAction::SensorRangeMinus:   OutKey = EKeys::F7;  return true;
    case EStarshatterInputAction::SensorRangePlus:    OutKey = EKeys::F8;  return true;
    case EStarshatterInputAction::EmconMinus:         OutKey = EKeys::F9;  return true;
    case EStarshatterInputAction::EmconPlus:          OutKey = EKeys::F10; return true;

        // Exit/pause
    case EStarshatterInputAction::ExitGame:       OutKey = EKeys::Escape;  return true;
    case EStarshatterInputAction::Pause:          OutKey = EKeys::Pause;   return true;

        // Time
    case EStarshatterInputAction::TimeExpand:     OutKey = EKeys::Delete;  return true;
    case EStarshatterInputAction::TimeCompress:   OutKey = EKeys::Insert;  return true;
    case EStarshatterInputAction::TimeSkip:       OutKey = EKeys::Insert;  bOutShift = true; return true;

        // Cameras
    case EStarshatterInputAction::CamBridge:      OutKey = EKeys::F1; return true;
    case EStarshatterInputAction::CamVirt:        OutKey = EKeys::F1; bOutShift = true; return true;
    case EStarshatterInputAction::CamChase:       OutKey = EKeys::F2; return true;
    case EStarshatterInputAction::CamDrop:        OutKey = EKeys::F2; bOutShift = true; return true;
    case EStarshatterInputAction::CamExtern:      OutKey = EKeys::F3; return true;
    case EStarshatterInputAction::TargetPadlock:  OutKey = EKeys::F4; return true;

        // HUD + dialogs
    case EStarshatterInputAction::ZoomWide:       OutKey = EKeys::K; return true;
    case EStarshatterInputAction::HudMode:        OutKey = EKeys::H; return true;
    case EStarshatterInputAction::HudColor:       OutKey = EKeys::H; bOutShift = true; return true;
    case EStarshatterInputAction::HudWarn:        OutKey = EKeys::C; return true;
    case EStarshatterInputAction::HudInst:        OutKey = EKeys::I; return true;
    case EStarshatterInputAction::NavDialog:      OutKey = EKeys::N; return true;
    case EStarshatterInputAction::WeaponDialog:   OutKey = EKeys::W; return true;
    case EStarshatterInputAction::EngineDialog:   OutKey = EKeys::E; return true;
    case EStarshatterInputAction::FlightDialog:   OutKey = EKeys::F; return true;
    case EStarshatterInputAction::RadioMenu:      OutKey = EKeys::R; return true;
    case EStarshatterInputAction::QuantumMenu:    OutKey = EKeys::Q; return true;

        // MFD (219 '[' , 221 ']')
    case EStarshatterInputAction::MFD1:           OutKey = EKeys::LeftBracket;  return true;
    case EStarshatterInputAction::MFD2:           OutKey = EKeys::RightBracket; return true;

        // Self destruct (ESC + SHIFT)
    case EStarshatterInputAction::SelfDestruct:   OutKey = EKeys::Escape; bOutShift = true; return true;

        // Swap roll/yaw
    case EStarshatterInputAction::SwapRollYaw:    OutKey = EKeys::J; return true;

        // Comms (VK_MENU == ALT)
    case EStarshatterInputAction::CommAttackTgt:      OutKey = EKeys::A; bOutAlt = true; return true;
    case EStarshatterInputAction::CommEscortTgt:      OutKey = EKeys::E; bOutAlt = true; return true;
    case EStarshatterInputAction::CommWepFree:        OutKey = EKeys::B; bOutAlt = true; return true;
    case EStarshatterInputAction::CommWepHold:        OutKey = EKeys::F; bOutAlt = true; return true;
    case EStarshatterInputAction::CommCoverMe:        OutKey = EKeys::H; bOutAlt = true; return true;
    case EStarshatterInputAction::CommSkipNav:        OutKey = EKeys::N; bOutAlt = true; return true;
    case EStarshatterInputAction::CommReturnToBase:   OutKey = EKeys::R; bOutAlt = true; return true;
    case EStarshatterInputAction::CommCallInbound:    OutKey = EKeys::I; bOutAlt = true; return true;
    case EStarshatterInputAction::CommRequestPicture: OutKey = EKeys::P; bOutAlt = true; return true;
    case EStarshatterInputAction::CommRequestSupport: OutKey = EKeys::S; bOutAlt = true; return true;

        // Chat (ALT+1..4)
    case EStarshatterInputAction::ChatBroadcast:  OutKey = EKeys::One;   bOutAlt = true; return true;
    case EStarshatterInputAction::ChatTeam:       OutKey = EKeys::Two;   bOutAlt = true; return true;
    case EStarshatterInputAction::ChatWing:       OutKey = EKeys::Three; bOutAlt = true; return true;
    case EStarshatterInputAction::ChatUnit:       OutKey = EKeys::Four;  bOutAlt = true; return true;

    default:
        break;
    }

    return false;
}

static FString FormatKeyStringWithMods(const FKey& Key, bool bShift, bool bCtrl, bool bAlt)
{
    if (!Key.IsValid())
        return TEXT("UNBOUND");

    FString S;
    if (bCtrl)  S += TEXT("CTRL+");
    if (bAlt)   S += TEXT("ALT+");
    if (bShift) S += TEXT("SHIFT+");
    S += Key.GetDisplayName().ToString();
    return S;
}

static bool IsModifierKey(const FKey& Key)
{
    return Key == EKeys::LeftShift || Key == EKeys::RightShift ||
        Key == EKeys::LeftControl || Key == EKeys::RightControl ||
        Key == EKeys::LeftAlt || Key == EKeys::RightAlt ||
        Key == EKeys::LeftCommand || Key == EKeys::RightCommand;
}

// ------------------------------------------------------------
// UKeyDlg
// ------------------------------------------------------------

UKeyDlg::UKeyDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
    bIsFocusable = true;
}

void UKeyDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // BaseScreen Enter/Escape routing:
    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegatesOnce();
}

void UKeyDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
    ApplyGlobalTheme(true, true, false); // buttons + text, no edits

    EnsureAutoVerticalBox();
    if (AutoVBox)
        AutoVBox->ClearChildren();
}

void UKeyDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegatesOnce();
    BuildKeyRows();

    bCapturing = false;
    bHasSelection = false;
    PendingRemaps.Reset();
    PendingClears.Reset();

    RefreshDisplayFromSettings();
    SetKeyboardFocus();
}

void UKeyDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    (void)MyGeometry;
    ExecFrame((double)InDeltaTime);
}

void UKeyDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling
}

void UKeyDlg::BindDelegatesOnce()
{
    if (bDelegatesBound)
        return;

    if (ClearButton)
        ClearButton->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnClearClicked);

    if (ApplyBtn)
        ApplyBtn->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnApplyClicked);

    if (CancelBtn)
        CancelBtn->OnClicked.AddUniqueDynamic(this, &UKeyDlg::OnCancelClicked);

    bDelegatesBound = true;
}

// ------------------------------------------------------------
// UI build
// ------------------------------------------------------------

void UKeyDlg::BuildKeyRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox || !WidgetTree)
    {
        UE_LOG(LogKeyDlg, Warning, TEXT("[KeyDlg] BuildKeyRows: AutoVBox/WidgetTree missing."));
        return;
    }

    VBox->ClearChildren();
    RowMap.Reset();
    ButtonToAction.Reset();
    SelectedRowButton.Reset();

    // -------------------------
    // Scroll list
    // -------------------------
    KeyScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("KeyScrollBox"));
    KeyListVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("KeyListVBox"));

    if (KeyScrollBox && KeyListVBox)
    {
        KeyScrollBox->ClearChildren();
        KeyListVBox->ClearChildren();

        KeyScrollBox->AddChild(KeyListVBox);

        // Add to AutoVBox with stable height
        UHorizontalBox* ScrollRow = AddRow(TEXT("KeyScrollRow"));
        if (ScrollRow)
        {
            USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("KeyScrollSize"));
            if (SB)
            {
                SB->SetHeightOverride(520.f);
                SB->AddChild(KeyScrollBox);

                if (UHorizontalBoxSlot* S = ScrollRow->AddChildToHorizontalBox(SB))
                {
                    S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    S->SetHorizontalAlignment(HAlign_Fill);
                    S->SetVerticalAlignment(VAlign_Fill);
                }
            }
            else
            {
                if (UHorizontalBoxSlot* S = ScrollRow->AddChildToHorizontalBox(KeyScrollBox))
                {
                    S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    S->SetHorizontalAlignment(HAlign_Fill);
                    S->SetVerticalAlignment(VAlign_Fill);
                }
            }
        }

        BuildKeyList();
    }

    // Buttons row (optional)
    if (ClearButton || ApplyBtn || CancelBtn)
    {
        UHorizontalBox* BtnRow = AddRow(TEXT("KeyButtonsRow"));
        if (BtnRow)
        {
            if (ClearButton) BtnRow->AddChildToHorizontalBox(ClearButton);
            if (ApplyBtn)    BtnRow->AddChildToHorizontalBox(ApplyBtn);
            if (CancelBtn)   BtnRow->AddChildToHorizontalBox(CancelBtn);
        }
    }
}

void UKeyDlg::ApplyStandardValueTextStyle(UTextBlock* Block) const
{
    if (!Block)
        return;

    // Requested: 0.5, 0.5, 0.5, 1.0
    Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)));
    Block->SetJustification(ETextJustify::Right);
}

void UKeyDlg::BuildKeyList()
{
    if (!KeyListVBox || !WidgetTree)
        return;

    KeyListVBox->ClearChildren();

    const UEnum* Enum = StaticEnum<EStarshatterInputAction>();
    if (!Enum)
        return;

    const int32 EnumCount = Enum->NumEnums();

    for (int32 i = 0; i < EnumCount; ++i)
    {
        const FString Name = Enum->GetNameStringByIndex(i);
        const int64 Value = Enum->GetValueByIndex(i);

        if (Name.EndsWith(TEXT("_MAX")) || Name.EndsWith(TEXT("MAX")))
            continue;

        if (!Enum->IsValidEnumValue(Value))
            continue;

        const EStarshatterInputAction Action = (EStarshatterInputAction)Value;

        // -----------------------------
        // Create Row Container
        // -----------------------------
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        if (!Row)
            continue;

       
        // -----------------------------
        // LEFT: Action Button (50%)
        // -----------------------------
        UButton* RowBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

        if (!RowBtn || !ActionText)
            continue;
        StyleButton_Default(RowBtn, 18);
        ActionText->SetText(FText::FromString(ActionToDisplayString(Action).ToUpper()));
        ActionText->SetColorAndOpacity(UITextColor);
        ActionText->SetJustification(ETextJustify::Left);

        RowBtn->SetContent(ActionText);
        RowBtn->OnPressed.AddUniqueDynamic(this, &UKeyDlg::OnRowButtonPressed);

        if (UHorizontalBoxSlot* LeftSlot = Row->AddChildToHorizontalBox(RowBtn))
        {
            LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LeftSlot->SetHorizontalAlignment(HAlign_Fill);
            LeftSlot->SetVerticalAlignment(VAlign_Center);
            LeftSlot->SetPadding(FMargin(0.f, 4.f, 8.f, 4.f));
        }

        // -----------------------------
        // RIGHT: Key Text (50%)
        // -----------------------------
        UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!KeyText)
            continue;

        ApplyStandardValueTextStyle(KeyText);
        KeyText->SetText(FText::FromString(TEXT("UNBOUND")));

        if (UHorizontalBoxSlot* RightSlot = Row->AddChildToHorizontalBox(KeyText))
        {
            RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            RightSlot->SetHorizontalAlignment(HAlign_Right);
            RightSlot->SetVerticalAlignment(VAlign_Center);
            RightSlot->SetPadding(FMargin(8.f, 4.f, 0.f, 4.f));
        }

        // -----------------------------
        // Add Row to VBox
        // -----------------------------
        if (UVerticalBoxSlot* VS = KeyListVBox->AddChildToVerticalBox(Row))
        {
            VS->SetPadding(FMargin(0.f, 2.f));
            VS->SetHorizontalAlignment(HAlign_Fill);
        }

        // -----------------------------
        // Store references
        // -----------------------------
        FKeyRowWidgets Widgets;
        Widgets.RowButton = RowBtn;
        Widgets.ActionText = ActionText;
        Widgets.KeyText = KeyText;

        RowMap.Add(Action, Widgets);
        ButtonToAction.Add(RowBtn, Action);
    }

    // Select first row by default
    if (!bHasSelection && RowMap.Num() > 0)
    {
        for (const TPair<EStarshatterInputAction, FKeyRowWidgets>& Pair : RowMap)
        {
            SelectAction(Pair.Key);
            break;
        }
    }
}

// ------------------------------------------------------------
// Selection + capture
// ------------------------------------------------------------

void UKeyDlg::OnRowButtonPressed()
{
    // Deterministic: find the pressed button
    for (const TPair<TWeakObjectPtr<UButton>, EStarshatterInputAction>& Pair : ButtonToAction)
    {
        UButton* B = Pair.Key.Get();
        if (!B) continue;

        if (B->IsPressed())
        {
            SelectAction(Pair.Value);
            return;
        }
    }

    // Fallback: keyboard focus (if it happens to work)
    for (const TPair<TWeakObjectPtr<UButton>, EStarshatterInputAction>& Pair : ButtonToAction)
    {
        UButton* B = Pair.Key.Get();
        if (!B) continue;

        if (B->HasKeyboardFocus())
        {
            SelectAction(Pair.Value);
            return;
        }
    }
}

void UKeyDlg::SelectAction(EStarshatterInputAction Action)
{
    SelectedAction = Action;
    bHasSelection = true;

    if (const FKeyRowWidgets* Row = RowMap.Find(Action))
        SelectedRowButton = Row->RowButton;

    BeginCaptureForSelected();
    SetKeyboardFocus();
}

void UKeyDlg::BeginCaptureForSelected()
{
    if (!bHasSelection)
        return;

    bCapturing = true;
    PendingClears.Remove(SelectedAction);
}

// ------------------------------------------------------------
// Key capture
// ------------------------------------------------------------

FReply UKeyDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey PressedKey = InKeyEvent.GetKey();

    // ESC = cancel/return
    if (PressedKey == EKeys::Escape || PressedKey == EKeys::Virtual_Back)
    {
        OnCancelClicked();
        return FReply::Handled();
    }

    if (!bCapturing || !bHasSelection)
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);

    if (!PressedKey.IsValid() || IsModifierKey(PressedKey))
        return FReply::Handled();

    PendingRemaps.Add(SelectedAction, PressedKey);
    PendingClears.Remove(SelectedAction);

    bCapturing = false;

    RefreshDisplayFromSettings();
    return FReply::Handled();
}

void UKeyDlg::HandleAccept() { OnApplyClicked(); }
void UKeyDlg::HandleCancel() { OnCancelClicked(); }

// ------------------------------------------------------------
// Buttons
// ------------------------------------------------------------

void UKeyDlg::OnClearClicked()
{
    if (!bHasSelection)
        return;

    PendingRemaps.Remove(SelectedAction);
    PendingClears.Add(SelectedAction);

    bCapturing = false;
    RefreshDisplayFromSettings();
}

void UKeyDlg::OnApplyClicked()
{
    CommitPendingToSettings();

    if (OptionsManager)
        OptionsManager->ReturnFromKeyDlg();
    else
        SetVisibility(ESlateVisibility::Collapsed);
}

void UKeyDlg::OnCancelClicked()
{
    PendingRemaps.Reset();
    PendingClears.Reset();
    bCapturing = false;

    if (OptionsManager)
        OptionsManager->ReturnFromKeyDlg();
    else
        SetVisibility(ESlateVisibility::Collapsed);
}

// ------------------------------------------------------------
// Commit + refresh
// ------------------------------------------------------------

void UKeyDlg::CommitPendingToSettings()
{
    UStarshatterKeyboardSettings* Settings = UStarshatterKeyboardSettings::Get();
    if (!Settings)
        return;

    Settings->Load();

    FStarshatterKeyboardConfig Cfg = Settings->GetKeyboardConfig();

    for (const EStarshatterInputAction A : PendingClears)
        Cfg.RemappedKeys.Remove(A);

    for (const TPair<EStarshatterInputAction, FKey>& Pair : PendingRemaps)
        Cfg.RemappedKeys.Add(Pair.Key, Pair.Value);

    Settings->SetKeyboardConfig(Cfg);
    Settings->Save();

    if (UStarshatterKeyboardSubsystem* KBSS = UStarshatterKeyboardSubsystem::Get(this))
        KBSS->ApplySettingsToRuntime(this);

    PendingRemaps.Reset();
    PendingClears.Reset();

    RefreshDisplayFromSettings();
}

void UKeyDlg::RefreshDisplayFromSettings()
{
    UStarshatterKeyboardSettings* Settings = UStarshatterKeyboardSettings::Get();
    if (!Settings)
        return;

    Settings->Load();
    const FStarshatterKeyboardConfig& Cfg = Settings->GetKeyboardConfig();

    for (TPair<EStarshatterInputAction, FKeyRowWidgets>& Pair : RowMap)
    {
        const EStarshatterInputAction Action = Pair.Key;
        FKeyRowWidgets& W = Pair.Value;

        if (!W.KeyText)
            continue;

        // 1) Pending clear
        if (PendingClears.Contains(Action))
        {
            W.KeyText->SetText(FText::FromString(TEXT("UNBOUND")));
            continue;
        }

        // 2) Pending remap
        if (const FKey* Pending = PendingRemaps.Find(Action))
        {
            W.KeyText->SetText(FText::FromString(KeyToDisplayString(*Pending)));
            continue;
        }

        // 3) Saved override
        if (const FKey* Saved = Cfg.RemappedKeys.Find(Action))
        {
            W.KeyText->SetText(FText::FromString(KeyToDisplayString(*Saved)));
            continue;
        }

        // 4) Default fallback (legacy defmap)
        {
            FKey DefKey;
            bool bShift = false, bCtrl = false, bAlt = false;
            if (GetDefaultKeyForAction(Action, DefKey, bShift, bCtrl, bAlt))
            {
                W.KeyText->SetText(FText::FromString(FormatKeyStringWithMods(DefKey, bShift, bCtrl, bAlt)));
                continue;
            }
        }

        // 5) No mapping
        W.KeyText->SetText(FText::FromString(TEXT("UNBOUND")));
    }
}

// ------------------------------------------------------------
// Display helpers
// ------------------------------------------------------------

FString UKeyDlg::ActionToDisplayString(EStarshatterInputAction Action)
{
    const UEnum* Enum = StaticEnum<EStarshatterInputAction>();
    if (Enum)
        return Enum->GetDisplayNameTextByValue((int64)Action).ToString();

    return TEXT("ACTION");
}

FString UKeyDlg::KeyToDisplayString(const FKey& Key)
{
    if (!Key.IsValid())
        return TEXT("UNBOUND");

    return Key.GetDisplayName().ToString();
}

bool UKeyDlg::IsModifierKey(const FKey& Key)
{
    return Key == EKeys::LeftShift || Key == EKeys::RightShift ||
        Key == EKeys::LeftControl || Key == EKeys::RightControl ||
        Key == EKeys::LeftAlt || Key == EKeys::RightAlt ||
        Key == EKeys::LeftCommand || Key == EKeys::RightCommand;
}