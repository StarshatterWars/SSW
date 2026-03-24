/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           OptionsScreen.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UOptionsScreen (Central Options Hub)

    - Owns all options sub-screens (Audio/Controls/Game/Joystick/Keyboard/Mods/Video)
    - Uses a WidgetSwitcher to swap sub-dialogs (instances already placed in WBP)
    - Tab buttons behave like radio buttons (selected tab disabled)
    - OPTION B: Border highlight (steel gray selected, alpha 0 unselected)
    - ESC returns to menu
    - TAB / SHIFT+TAB cycles tabs and changes active page

    IMPORTANT UE NOTES
    ==================
    - UButton does NOT have SetIsFocusable(). Remove any usage.
    - Focus is achieved by:
        * bIsFocusable = true on this widget
        * Setting InputMode UIOnly focus to BtnAudio's slate widget (or TakeWidget())
        * Using SetUserFocus / SetKeyboardFocus on tab buttons

=============================================================================*/

#include "OptionsScreen.h"

// UMG
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "OptionsManagedPage.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/UserWidget.h"

// Input
#include "Input/Reply.h"
#include "InputCoreTypes.h"

// Menu routing
#include "MenuScreen.h"

DEFINE_LOG_CATEGORY_STATIC(LogOptionsScreen, Log, All);

UOptionsScreen::UOptionsScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsFocusable = true;
    SetDialogInputEnabled(true);
}

void UOptionsScreen::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BindDelegates();
}

void UOptionsScreen::NativeConstruct()
{
    Super::NativeConstruct();

    bIsFocusable = true;

    BindDelegates();
    // Wire the manager into EVERY page sitting in the switcher:
    if (OptionsSwitcher)
    {
        const int32 Count = OptionsSwitcher->GetNumWidgets();
        for (int32 i = 0; i < Count; ++i)
        {
            if (UWidget* W = OptionsSwitcher->GetWidgetAtIndex(i))
            {
                if (W->Implements<UOptionsManagedPage>())
                {
                    IOptionsManagedPage::Execute_SetOptionsManager(W, this);
                }
            }
        }
    }

    // Default tab
    ShowAudDlg();

    if (TitleText)
        TitleText->SetText(FText::FromString(TEXT("OPTIONS")));

    // Force focus so NativeOnKeyDown receives TAB/ESC
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetShowMouseCursor(true);
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeUIOnly Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        // Focus the first tab button if possible, otherwise the screen itself:
        if (BtnAudio)
            Mode.SetWidgetToFocus(BtnAudio->TakeWidget());
        else
            Mode.SetWidgetToFocus(TakeWidget());

        PC->SetInputMode(Mode);

        // Also set user focus (avoids “does not support focus” warnings)
        if (BtnAudio)
            BtnAudio->SetUserFocus(PC);
        else
            SetUserFocus(PC);
    }
}

void UOptionsScreen::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Tabs (alphabetical):
    if (BtnAudio)    BtnAudio->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnAudioClicked);
    if (BtnControls) BtnControls->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnControlsClicked);
    if (BtnGame)     BtnGame->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnGameClicked);
    if (BtnJoystick) BtnJoystick->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnJoystickClicked);
    if (BtnKeyboard) BtnKeyboard->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnKeyboardClicked);
    if (BtnMods)     BtnMods->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnModsClicked);
    if (BtnVideo)    BtnVideo->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnVideoClicked);

    // Bottom buttons (optional):
    if (BtnSave)     BtnSave->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnSaveClicked);
    if (BtnCancel)   BtnCancel->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnCancelClicked);

    const bool bAny =
        (BtnAudio || BtnControls || BtnGame || BtnJoystick || BtnKeyboard || BtnMods || BtnVideo || BtnSave || BtnCancel);

    if (!bAny)
    {
        UE_LOG(LogOptionsScreen, Error, TEXT("[Options] NO BUTTONS BOUND. Check WBP names (BtnAudio, BtnControls, ...) and IsVariable."));
    }

    bDelegatesBound = true;
}

void UOptionsScreen::HandleCancel()
{
    // BaseScreen ESC fallback
    CancelOptions();
}

FReply UOptionsScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    (void)InGeometry;

    const FKey Key = InKeyEvent.GetKey();

    // ESC -> return to menu
    if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
    {
        CancelOptions();
        return FReply::Handled();
    }

    // TAB cycles tabs (Shift+Tab reverses)
    if (Key == EKeys::Tab)
    {
        const bool bShift = InKeyEvent.IsShiftDown();
        FocusTabByDelta(bShift ? -1 : +1);
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ------------------------------------------------------------
// Switcher helper (uses existing instances in WBP)
// ------------------------------------------------------------

void UOptionsScreen::SwitchToWidget(UWidget* Widget)
{
    if (!OptionsSwitcher)
    {
        UE_LOG(LogOptionsScreen, Warning, TEXT("[Options] OptionsSwitcher is NULL (BindWidgetOptional missing in WBP)."));
        return;
    }

    if (!Widget)
    {
        UE_LOG(LogOptionsScreen, Warning, TEXT("[Options] SwitchToWidget: Widget is NULL"));
        return;
    }

    OptionsSwitcher->SetActiveWidget(Widget);
}

// ------------------------------------------------------------
// Tab visuals
// ------------------------------------------------------------

void UOptionsScreen::SetActiveTab(UButton* ActiveButton)
{
    // Radio behavior: selected tab disabled (lets Disabled style look “selected” if you want)
    if (BtnAudio)    BtnAudio->SetIsEnabled(BtnAudio != ActiveButton);
    if (BtnControls) BtnControls->SetIsEnabled(BtnControls != ActiveButton);
    if (BtnGame)     BtnGame->SetIsEnabled(BtnGame != ActiveButton);
    if (BtnJoystick) BtnJoystick->SetIsEnabled(BtnJoystick != ActiveButton);
    if (BtnKeyboard) BtnKeyboard->SetIsEnabled(BtnKeyboard != ActiveButton);
    if (BtnMods)     BtnMods->SetIsEnabled(BtnMods != ActiveButton);
    if (BtnVideo)    BtnVideo->SetIsEnabled(BtnVideo != ActiveButton);

    // OPTION B: border highlight
    UpdateTabBorders(ActiveButton);
}

void UOptionsScreen::SetBorderSelected(UBorder* Border, bool bSelected)
{
    if (!Border)
        return;

    // Selected: steel gray visible
    // Unselected: alpha = 0 (invisible)
    const FLinearColor SelectedColor(0.45f, 0.48f, 0.52f, 1.0f);
    const FLinearColor UnselectedColor(0.45f, 0.48f, 0.52f, 0.0f);

    Border->SetBrushColor(bSelected ? SelectedColor : UnselectedColor);
}

void UOptionsScreen::UpdateTabBorders(UButton* ActiveButton)
{
    SetBorderSelected(BorderAudio, ActiveButton == BtnAudio);
    SetBorderSelected(BorderControls, ActiveButton == BtnControls);
    SetBorderSelected(BorderGame, ActiveButton == BtnGame);
    SetBorderSelected(BorderJoystick, ActiveButton == BtnJoystick);
    SetBorderSelected(BorderKeyboard, ActiveButton == BtnKeyboard);
    SetBorderSelected(BorderMods, ActiveButton == BtnMods);
    SetBorderSelected(BorderVideo, ActiveButton == BtnVideo);
}

// ------------------------------------------------------------
// TAB navigation (cycles + switches pages)
// ------------------------------------------------------------

int32 UOptionsScreen::GetCurrentTabIndex() const
{
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick, BtnKeyboard, BtnMods, BtnVideo };

    // Prefer actual keyboard focus:
    for (int32 i = 0; i < UE_ARRAY_COUNT(Tabs); ++i)
        if (Tabs[i] && Tabs[i]->HasKeyboardFocus())
            return i;

    // Fallback: infer from “selected tab disabled” state
    for (int32 i = 0; i < UE_ARRAY_COUNT(Tabs); ++i)
        if (Tabs[i] && !Tabs[i]->GetIsEnabled())
            return i;

    return 0;
}

void UOptionsScreen::FocusTabByDelta(int32 Delta)
{
    const int32 Count = 7;
    int32 Cur = GetCurrentTabIndex();
    int32 Next = Cur;

    for (int32 step = 0; step < Count; ++step)
    {
        Next = (Next + Delta + Count) % Count;

        UButton* Candidate = nullptr;
        switch (Next)
        {
        case 0: Candidate = BtnAudio; break;
        case 1: Candidate = BtnControls; break;
        case 2: Candidate = BtnGame; break;
        case 3: Candidate = BtnJoystick; break;
        case 4: Candidate = BtnKeyboard; break;
        case 5: Candidate = BtnMods; break;
        case 6: Candidate = BtnVideo; break;
        default: break;
        }

        if (!Candidate)
            continue;

        // Switch pages + update highlight
        switch (Next)
        {
        case 0: ShowAudDlg(); break;
        case 1: ShowCtlDlg(); break;
        case 2: ShowGameDlg(); break;
        case 3: ShowJoyDlg(); break;
        case 4: ShowKeyDlg(); break;
        case 5: ShowModDlg(); break;
        case 6: ShowVidDlg(); break;
        default: break;
        }

        Candidate->SetKeyboardFocus();
        return;
    }
}

// ------------------------------------------------------------
// Click handlers
// ------------------------------------------------------------

void UOptionsScreen::OnAudioClicked() { ShowAudDlg(); }
void UOptionsScreen::OnControlsClicked() { ShowCtlDlg(); }
void UOptionsScreen::OnGameClicked() { ShowGameDlg(); }
void UOptionsScreen::OnJoystickClicked() { ShowJoyDlg(); }
void UOptionsScreen::OnKeyboardClicked() { ShowKeyDlg(); }
void UOptionsScreen::OnModsClicked() { ShowModDlg(); }
void UOptionsScreen::OnVideoClicked() { ShowVidDlg(); }

void UOptionsScreen::OnSaveClicked() { ApplyOptions(); }
void UOptionsScreen::OnCancelClicked() { CancelOptions(); }

// ------------------------------------------------------------
// Public routing (uses instances already bound in WBP)
// ------------------------------------------------------------

void UOptionsScreen::ShowAudDlg()
{
    SetActiveTab(BtnAudio);
    SwitchToWidget(AudioDlg.Get());          // IMPORTANT: .Get() on TObjectPtr

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnAudio) BtnAudio->SetUserFocus(PC);
}

void UOptionsScreen::ShowCtlDlg()
{
    SetActiveTab(BtnControls);
    SwitchToWidget(ControlsDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnControls) BtnControls->SetUserFocus(PC);
}

void UOptionsScreen::ShowGameDlg()
{
    SetActiveTab(BtnGame);
    SwitchToWidget(GameDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnGame) BtnGame->SetUserFocus(PC);
}

void UOptionsScreen::ShowJoyDlg()
{
    SetActiveTab(BtnJoystick);
    SwitchToWidget(JoystickDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnJoystick) BtnJoystick->SetUserFocus(PC);
}

void UOptionsScreen::ShowKeyDlg()
{
    SetActiveTab(BtnKeyboard);
    SwitchToWidget(KeyboardDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnKeyboard) BtnKeyboard->SetUserFocus(PC);
}

void UOptionsScreen::ShowModDlg()
{
    SetActiveTab(BtnMods);
    SwitchToWidget(ModsDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnMods) BtnMods->SetUserFocus(PC);
}

void UOptionsScreen::ShowVidDlg()
{
    SetActiveTab(BtnVideo);
    SwitchToWidget(VideoDlg.Get());

    if (APlayerController* PC = GetOwningPlayer())
        if (BtnVideo) BtnVideo->SetUserFocus(PC);
}

// ------------------------------------------------------------
// Apply / Cancel orchestration
// ------------------------------------------------------------

void UOptionsScreen::ApplyOptions()
{
    UE_LOG(LogOptionsScreen, Log, TEXT("[Options] ApplyOptions()"));

    // NOTE:
    // Right now, your subdialogs (AudioDlg/VideoDlg/etc.) already push settings
    // when their Apply button is pressed (often via Manager->ApplyOptions()).
    // If you later remove “Save” and auto-apply on tab switch, we’ll call Apply()
    // on the page being left inside FocusTabByDelta / ShowXxx methods.
}

void UOptionsScreen::CancelOptions()
{
    UE_LOG(LogOptionsScreen, Log, TEXT("[Options] CancelOptions()"));

    // Return to menu
    if (UMenuScreen* Menu = GetMenuManager())
    {
        Menu->ReturnFromOptions();
    }
    else
    {
        UE_LOG(LogOptionsScreen, Warning, TEXT("[Options] CancelOptions: MenuManager is NULL"));
        RemoveFromParent(); // safe fallback so you don’t get stuck
    }
}

void UOptionsScreen::ReturnFromKeyDlg()
{
    // KeyDlg should call back here.
    // Return to Keyboard tab/page by default (matches your current request).
    ShowKeyDlg();
    SetKeyboardFocus();
}
