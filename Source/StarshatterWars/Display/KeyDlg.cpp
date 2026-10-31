
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
#include "StarshatterInputBindings.h"
#include "StarshatterControlsSettings.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "StarshatterInputLabels.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
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
    PendingClears.Reset(); PendingEnhanced.Reset(); PendingEnhancedClears.Reset();

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
    EnhancedRows.Reset(); BindingContexts.Reset(); CategoryBoxes.Reset();
    bHasSelection=false; bCapturing=false; SelectedEnhancedId.Reset();
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

UVerticalBox* UKeyDlg::CategoryBox(const FString& Category)
{
    if (auto* Found=CategoryBoxes.Find(Category)) return Found->Get();
    auto* Header=WidgetTree->ConstructWidget<UTextBlock>();
    Header->SetText(FText::FromString(Category));
    Header->SetColorAndOpacity(FLinearColor(0.2f,0.85f,1.f));
    auto Font=Header->GetFont();Font.Size=20;Header->SetFont(Font);
    KeyListVBox->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,18,0,6));
    auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();
    KeyListVBox->AddChildToVerticalBox(Box);
    CategoryBoxes.Add(Category,Box);return Box;
}

void UKeyDlg::BuildKeyList()
{
    if (!KeyListVBox || !WidgetTree) return;
    KeyListVBox->ClearChildren();CategoryBoxes.Reset();
    RowMap.Reset();ButtonToAction.Reset();EnhancedRows.Reset();BindingContexts.Reset();
    bHasSelection=false;bCapturing=false;SelectedEnhancedId.Reset();
    auto* Help=WidgetTree->ConstructWidget<UTextBlock>();
    Help->SetText(FText::FromString(TEXT("SELECT A CONTROL TO REBIND. ESC CANCELS CAPTURE. APPLY SAVES CHANGES.")));
    Help->SetAutoWrapText(true);Help->SetColorAndOpacity(UITextColor);
    KeyListVBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0,0,0,8));
    const TCHAR* Categories[]={TEXT("MOVEMENT & FLIGHT"),TEXT("CAMERA & VIEWS"),TEXT("WEAPONS"),TEXT("TARGETING"),TEXT("SENSORS"),TEXT("NAVIGATION & SHIP SYSTEMS"),TEXT("HUD & PANELS"),TEXT("COMMUNICATIONS"),TEXT("MODIFIER KEYS"),TEXT("GENERAL")};
    for (const TCHAR* Category:Categories) CategoryBox(Category);
    AppendFighterHUDBindings();
    const auto* Enum=StaticEnum<EStarshatterInputAction>();
    for (int32 Index=0;Index<Enum->NumEnums();++Index)
    {
        const FString Name=Enum->GetNameStringByIndex(Index);
        if (Name.EndsWith(TEXT("MAX"))) continue;
        const auto Action=static_cast<EStarshatterInputAction>(Enum->GetValueByIndex(Index));
        if (Action==EStarshatterInputAction::None || Action==EStarshatterInputAction::Action0 || Action==EStarshatterInputAction::Action1 ||
            Action==EStarshatterInputAction::TargetNext || Action==EStarshatterInputAction::TargetPrevious ||
            Action==EStarshatterInputAction::CameraChase || Action==EStarshatterInputAction::CameraExternal) continue;
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Button=WidgetTree->ConstructWidget<UButton>();
        auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
        auto* Key=WidgetTree->ConstructWidget<UTextBlock>();
        StyleButton_Default(Button,18);
        FKey OriginalKey;bool Shift=false,Control=false,Alt=false;
        if(!GetDefaultKeyForAction(Action,OriginalKey,Shift,Control,Alt)) continue;
        FString Caption=ActionToDisplayString(Action).ToUpper();
        if(OriginalKey.IsValid()) Caption+=TEXT("  [")+FormatKeyStringWithMods(OriginalKey,Shift,Control,Alt).ToUpper()+TEXT("]");
        Label->SetText(FText::FromString(Caption));Label->SetColorAndOpacity(UITextColor);
        Label->SetAutoWrapText(true);Button->SetContent(Label);
        Button->OnPressed.AddUniqueDynamic(this,&UKeyDlg::OnRowButtonPressed);
        auto* RowSlot=Row->AddChildToHorizontalBox(Button);FSlateChildSize Size(ESlateSizeRule::Fill);Size.Value=3;RowSlot->SetSize(Size);RowSlot->SetPadding(FMargin(0,3,20,3));
        ApplyStandardValueTextStyle(Key);
        RowSlot=Row->AddChildToHorizontalBox(Key);RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));RowSlot->SetVerticalAlignment(VAlign_Center);
        CategoryBox(StarshatterInputBindings::Category(Name))->AddChildToVerticalBox(Row);
        FKeyRowWidgets Widgets;Widgets.RowButton=Button;Widgets.ActionText=Label;Widgets.KeyText=Key;
        RowMap.Add(Action,Widgets);ButtonToAction.Add(Button,Action);
    }
}

void UKeyDlg::AppendFighterHUDBindings()
{
    TArray<UInputMappingContext*> Contexts;
    const TCHAR* Paths[]={TEXT("/Game/Input/IMC_Game.IMC_Game"),TEXT("/Game/Input/IMC_FighterHUD.IMC_FighterHUD")};
    for (const TCHAR* Path:Paths) if(auto* Context=LoadObject<UInputMappingContext>(nullptr,Path)) Contexts.AddUnique(Context);
    auto* Controls=UStarshatterControlsSettings::Get();
    const TSoftObjectPtr<UInputMappingContext> Models[]={Controls->GetArcadeContext(),Controls->GetFlightSimContext(),Controls->GetHybridContext()};
    for (const auto& Model:Models) if (!Model.IsNull()) if(auto* Context=Model.LoadSynchronous()) Contexts.AddUnique(Context);
    Contexts.Add(StarshatterInputBindings::SupplementalContext(this));
    for (auto* Context:Contexts)
    {
        BindingContexts.Add(Context);
        for (const auto& Mapping:Context->GetMappings())
        {
            if (!Mapping.Action) continue;
            const FString Id=StarshatterInputBindings::Id(Mapping);
            if (EnhancedRows.Contains(Id)) continue;
            auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* Button=WidgetTree->ConstructWidget<UButton>();
            auto* Label=WidgetTree->ConstructWidget<UTextBlock>();
            auto* Key=WidgetTree->ConstructWidget<UTextBlock>();
            StyleButton_Default(Button,18);
            FString Text=UStarshatterInputLabels::GetActionLabel(Mapping.Action).ToString().ToUpper();
            // Original key distinguishes independently remappable directions/alternate bindings.
            Text+=TEXT("  [")+Mapping.Key.GetDisplayName().ToString().ToUpper()+TEXT("]");
            Label->SetText(FText::FromString(Text));Label->SetColorAndOpacity(UITextColor);Label->SetAutoWrapText(true);
            Button->SetContent(Label);Button->OnPressed.AddUniqueDynamic(this,&UKeyDlg::OnRowButtonPressed);
            auto* RowSlot=Row->AddChildToHorizontalBox(Button);FSlateChildSize Size(ESlateSizeRule::Fill);Size.Value=3;RowSlot->SetSize(Size);RowSlot->SetPadding(FMargin(0,3,20,3));
            ApplyStandardValueTextStyle(Key);RowSlot=Row->AddChildToHorizontalBox(Key);RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));RowSlot->SetVerticalAlignment(VAlign_Center);
            CategoryBox(StarshatterInputBindings::Category(Mapping.Action->GetName()))->AddChildToVerticalBox(Row);
            FEnhancedRow Item;Item.Mapping=Mapping;Item.Button=Button;Item.KeyText=Key;EnhancedRows.Add(Id,Item);
        }
    }
}

void UKeyDlg::OnRowButtonPressed()
{
    for (const auto& Pair:EnhancedRows)
        if (Pair.Value.Button && Pair.Value.Button->IsPressed())
        {
            SelectedEnhancedId=Pair.Key;bHasSelection=true;SelectedRowButton=Pair.Value.Button;
            BeginCaptureForSelected();SetKeyboardFocus();return;
        }
    for (const auto& Pair:ButtonToAction)
        if (Pair.Key.IsValid() && Pair.Key->IsPressed()) {SelectAction(Pair.Value);return;}
}
void UKeyDlg::SelectAction(EStarshatterInputAction Action)
{
    SelectedEnhancedId.Reset();SelectedAction=Action;bHasSelection=true;
    if (auto* Row=RowMap.Find(Action)) SelectedRowButton=Row->RowButton;
    BeginCaptureForSelected();SetKeyboardFocus();
}
void UKeyDlg::BeginCaptureForSelected()
{
    if (!bHasSelection) return;
    bCapturing=true;CaptureStarted=FPlatformTime::Seconds();CaptureMouseDelta=FVector2D::ZeroVector;
    RefreshDisplayFromSettings();
}
bool UKeyDlg::CaptureKey(FKey Key)
{
    if (!bCapturing || !bHasSelection || !Key.IsValid()) return false;
    if (SelectedEnhancedId.IsEmpty() && (Key.IsMouseButton() || Key.IsGamepadKey() || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())) return false;
    if (SelectedEnhancedId.IsEmpty()) {PendingRemaps.Add(SelectedAction,Key);PendingClears.Remove(SelectedAction);}
    else {PendingEnhanced.Add(SelectedEnhancedId,Key);PendingEnhancedClears.Remove(SelectedEnhancedId);}
    bCapturing=false;RefreshDisplayFromSettings();return true;
}
FReply UKeyDlg::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Escape || Event.GetKey()==EKeys::Virtual_Back)
    {
        if (bCapturing) {bCapturing=false;RefreshDisplayFromSettings();}
        else OnCancelClicked();
        return FReply::Handled();
    }
    return CaptureKey(Event.GetKey())?FReply::Handled():Super::NativeOnKeyDown(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    return CaptureKey(Event.GetEffectingButton())?FReply::Handled():Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseWheel(const FGeometry& Geometry,const FPointerEvent& Event)
{
    FKey Key=Event.GetWheelDelta()>0?EKeys::MouseScrollUp:EKeys::MouseScrollDown;
    if (const auto* Row=EnhancedRows.Find(SelectedEnhancedId))
        if (Row->Mapping.Action && Row->Mapping.Action->ValueType!=EInputActionValueType::Boolean) Key=EKeys::MouseWheelAxis;
    return CaptureKey(Key)?FReply::Handled():Super::NativeOnMouseWheel(Geometry,Event);
}
FReply UKeyDlg::NativeOnMouseMove(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (bCapturing && FPlatformTime::Seconds()-CaptureStarted>0.35)
        if (const auto* Row=EnhancedRows.Find(SelectedEnhancedId))
            if (Row->Mapping.Action && Row->Mapping.Action->ValueType!=EInputActionValueType::Boolean)
            {
                CaptureMouseDelta+=Event.GetCursorDelta();
                if (CaptureMouseDelta.GetAbsMax()>30)
                    if (CaptureKey(FMath::Abs(CaptureMouseDelta.X)>FMath::Abs(CaptureMouseDelta.Y)?EKeys::MouseX:EKeys::MouseY)) return FReply::Handled();
            }
    return Super::NativeOnMouseMove(Geometry,Event);
}
FReply UKeyDlg::NativeOnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event)
{
    return FMath::Abs(Event.GetAnalogValue())>0.6f && CaptureKey(Event.GetKey())?FReply::Handled():Super::NativeOnAnalogValueChanged(Geometry,Event);
}
void UKeyDlg::HandleAccept() {OnApplyClicked();}
void UKeyDlg::HandleCancel() {OnCancelClicked();}
void UKeyDlg::OnClearClicked()
{
    if (!bHasSelection) return;
    if (SelectedEnhancedId.IsEmpty()) {PendingRemaps.Remove(SelectedAction);PendingClears.Add(SelectedAction);}
    else {PendingEnhanced.Remove(SelectedEnhancedId);PendingEnhancedClears.Add(SelectedEnhancedId);}
    bCapturing=false;RefreshDisplayFromSettings();
}
void UKeyDlg::Apply() {CommitPendingToSettings();}
void UKeyDlg::Cancel()
{
    PendingRemaps.Reset();PendingClears.Reset();PendingEnhanced.Reset();PendingEnhancedClears.Reset();bCapturing=false;
    RefreshDisplayFromSettings();
}
void UKeyDlg::OnApplyClicked()
{
    CommitPendingToSettings();
    if (OptionsManager) OptionsManager->ReturnFromKeyDlg();else SetVisibility(ESlateVisibility::Collapsed);
}
void UKeyDlg::OnCancelClicked()
{
    Cancel();if (OptionsManager) OptionsManager->ReturnFromKeyDlg();else SetVisibility(ESlateVisibility::Collapsed);
}
void UKeyDlg::CommitPendingToSettings()
{
    auto* Settings=UStarshatterKeyboardSettings::Get();Settings->Load();
    auto Config=Settings->GetKeyboardConfig();
    for (auto Action:PendingClears) Config.RemappedKeys.Remove(Action);
    for (const auto& Pair:PendingRemaps) Config.RemappedKeys.Add(Pair.Key,Pair.Value);
    for (const auto& Id:PendingEnhancedClears) Config.EnhancedBindings.Remove(Id);
    for (const auto& Pair:PendingEnhanced) Config.EnhancedBindings.Add(Pair.Key,Pair.Value);
    Settings->SetKeyboardConfig(Config);Settings->Save();
    if (auto* Subsystem=UStarshatterKeyboardSubsystem::Get(this)) Subsystem->ApplySettingsToRuntime(this);
    PendingRemaps.Reset();PendingClears.Reset();PendingEnhanced.Reset();PendingEnhancedClears.Reset();bCapturing=false;
    RefreshDisplayFromSettings();
}
void UKeyDlg::RefreshDisplayFromSettings()
{
    auto* Settings=UStarshatterKeyboardSettings::Get();Settings->Load();const auto& Config=Settings->GetKeyboardConfig();
    for (auto& Pair:RowMap)
    {
        FKey Key;bool Shift=false,Ctrl=false,Alt=false;
        GetDefaultKeyForAction(Pair.Key,Key,Shift,Ctrl,Alt);
        if (!PendingClears.Contains(Pair.Key))
        {
            if (const auto* Saved=Config.RemappedKeys.Find(Pair.Key)) Key=*Saved;
            if (const auto* Pending=PendingRemaps.Find(Pair.Key)) Key=*Pending;
        }
        const FString Text=bCapturing && SelectedEnhancedId.IsEmpty() && SelectedAction==Pair.Key?TEXT("PRESS A KEY..."):FormatKeyStringWithMods(Key,Shift,Ctrl,Alt);
        Pair.Value.KeyText->SetText(FText::FromString(Text));
    }
    for (auto& Pair:EnhancedRows)
    {
        auto Mapping=Pair.Value.Mapping;
        if (!PendingEnhancedClears.Contains(Pair.Key))
        {
            if (const auto* Saved=Config.EnhancedBindings.Find(Pair.Key)) Mapping.Key=*Saved;
            if (const auto* Pending=PendingEnhanced.Find(Pair.Key)) Mapping.Key=*Pending;
        }
        Pair.Value.KeyText->SetText(bCapturing && SelectedEnhancedId==Pair.Key?FText::FromString(TEXT("PRESS A KEY...")):UStarshatterInputLabels::GetBindingKeyLabel(Mapping).ToUpper());
    }
}
FString UKeyDlg::ActionToDisplayString(EStarshatterInputAction Action) {return UStarshatterInputLabels::GetLegacyActionLabel(Action).ToString();}
FString UKeyDlg::KeyToDisplayString(const FKey& Key) {return Key.IsValid()?Key.GetDisplayName().ToString():TEXT("UNBOUND");}
bool UKeyDlg::IsModifierKey(const FKey& Key) {return Key.IsModifierKey();}

FReply UKeyDlg::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    return CaptureKey(Event.GetEffectingButton())?FReply::Handled():Super::NativeOnPreviewMouseButtonDown(Geometry,Event);
}
