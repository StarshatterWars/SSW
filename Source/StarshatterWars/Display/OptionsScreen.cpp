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
#include "Components/SizeBox.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/ButtonSlot.h"
#include "MissionUIStyle.h"
#include "StarshatterUIStyleSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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

    BuildCreditsPage();
    StandardizeTopTabs();
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

void UOptionsScreen::StandardizeTopTabs()
{
    // Use one shared style and one label treatment for all eight top tabs.
    // The seven pre-existing WBP buttons keep their click bindings and parents.
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick,
                        BtnKeyboard, BtnMods, BtnVideo, BtnCredits };
    const TCHAR* Names[] = { TEXT("AUDIO"), TEXT("CONTROLS"), TEXT("GAME"),
                             TEXT("JOYSTICK"), TEXT("KEYBOARD"), TEXT("MODS"),
                             TEXT("VIDEO"), TEXT("CREDITS") };

    UStarshatterUIStyleSubsystem* SharedStyle = nullptr;
    if (UGameInstance* GI = GetGameInstance())
        SharedStyle = GI->GetSubsystem<UStarshatterUIStyleSubsystem>();

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tabs); ++Index)
    {
        UButton* Tab = Tabs[Index];
        if (!Tab) continue;

        if (SharedStyle)
            SharedStyle->ApplyMenuButtonStyle(Tab);

        // Normalize existing WBP TextBlocks as well as the dynamically built Credits tab.
        UTextBlock* Label = Cast<UTextBlock>(Tab->GetContent());
        if (!Label && Tab->GetContent())
        {
            if (USizeBox* Wrapper = Cast<USizeBox>(Tab->GetContent()))
                Label = Cast<UTextBlock>(Wrapper->GetContent());
        }
        if (Label)
        {
            Label->SetText(FText::FromString(Names[Index]));
            Label->SetFont(MissionUIStyle::GetInfoLabelFont(13));
            Label->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
            Label->SetJustification(ETextJustify::Center);
        }

        if (UButtonSlot* ButtonContentSlot = Cast<UButtonSlot>(Tab->GetContent() ? Tab->GetContent()->Slot : nullptr))
        {
            ButtonContentSlot->SetHorizontalAlignment(HAlign_Center);
            ButtonContentSlot->SetVerticalAlignment(VAlign_Center);
            ButtonContentSlot->SetPadding(FMargin(10.f, 8.f));
        }

    }
    UE_LOG(LogOptionsScreen, Log, TEXT("[Options] Standardized all 8 top tab buttons."));
}

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
    if (BtnCredits)  BtnCredits->SetIsEnabled(BtnCredits != ActiveButton);

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
    SetBorderSelected(BorderCredits, ActiveButton == BtnCredits);
}

// ------------------------------------------------------------
// TAB navigation (cycles + switches pages)
// ------------------------------------------------------------

int32 UOptionsScreen::GetCurrentTabIndex() const
{
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick, BtnKeyboard, BtnMods, BtnVideo, BtnCredits };

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
    const int32 Count = 8;
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
        case 7: Candidate = BtnCredits; break;
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
        case 7: ShowCreditsDlg(); break;
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
void UOptionsScreen::OnCreditsClicked() { ShowCreditsDlg(); }

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


void UOptionsScreen::BuildCreditsPage()
{
    if (!OptionsSwitcher || !WidgetTree || CreditsPage) return;

    UBorder* Page = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Options_CreditsPage"));
    Page->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.083f, 1.f));
    Page->SetPadding(FMargin(22.f));
    CreditsPage = Page;
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Page->SetContent(Content);

    UBorder* HeadingBand = WidgetTree->ConstructWidget<UBorder>();
    HeadingBand->SetBrushColor(FLinearColor(0.105f, 0.113f, 0.125f, 1.f));
    HeadingBand->SetPadding(FMargin(16.f, 10.f));
    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
    Heading->SetText(FText::FromString(TEXT("ABOUT / CREDITS")));
    Heading->SetFont(MissionUIStyle::GetHeaderFont(30));
    Heading->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    HeadingBand->SetContent(Heading);
    Content->AddChildToVerticalBox(HeadingBand)->SetPadding(FMargin(0.f, 0.f, 0.f, 15.f));

    UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>();
    Caption->SetText(FText::FromString(TEXT("STARSHATTER WARS  /  DEVELOPMENT AND ACKNOWLEDGEMENTS")));
    Caption->SetFont(MissionUIStyle::GetInfoLabelFont(13));
    Caption->SetColorAndOpacity(FSlateColor(FLinearColor(0.16f, 0.77f, 0.91f, 1.f)));
    Content->AddChildToVerticalBox(Caption)->SetPadding(FMargin(0.f, 0.f, 0.f, 13.f));

    CreditsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Options_CreditsScroll"));
    CreditsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Content->AddChildToVerticalBox(CreditsScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CreditsText = WidgetTree->ConstructWidget<UTextBlock>();
    CreditsText->SetFont(MissionUIStyle::GetRowFont(16));
    CreditsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CreditsText->SetAutoWrapText(true);
    CreditsScroll->AddChild(CreditsText);
    ReloadCredits();
    OptionsSwitcher->AddChild(Page);

    // Append the Credits tab to the same tab strip that owns Video.
    UWidget* TabAncestorWidget = BorderVideo ? static_cast<UWidget*>(BorderVideo.Get()) : static_cast<UWidget*>(BtnVideo.Get());
    UPanelWidget* TabPanel = nullptr;
    while (TabAncestorWidget && !TabPanel)
    {
        UPanelWidget* Parent = TabAncestorWidget->GetParent();
        if (!Parent) break;
        if (Cast<UHorizontalBox>(Parent) || Cast<UVerticalBox>(Parent)) TabPanel = Parent;
        TabAncestorWidget = Parent;
    }
    if (!TabPanel)
    {
        UE_LOG(LogOptionsScreen, Error, TEXT("[OptionsCredits] Video tab strip not found. Check WBP tab panel."));
        return;
    }
    BorderCredits = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Options_BorderCredits"));
    BorderCredits->SetBrushColor(FLinearColor(0.45f, 0.48f, 0.52f, 0.f));
    BorderCredits->SetPadding(FMargin(2.f));
    BtnCredits = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Options_BtnCredits"));
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
    ButtonText->SetText(FText::FromString(TEXT("CREDITS")));
    ButtonText->SetFont(MissionUIStyle::GetInfoLabelFont(13));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    if (UButtonSlot* CreditsButtonSlot = Cast<UButtonSlot>(BtnCredits->AddChild(ButtonText)))
    {
        CreditsButtonSlot->SetPadding(FMargin(10.f, 8.f));
        CreditsButtonSlot->SetHorizontalAlignment(HAlign_Center);
        CreditsButtonSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (UGameInstance* GI = GetGameInstance())
        if (UStarshatterUIStyleSubsystem* Styles = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
            Styles->ApplyMenuButtonStyle(BtnCredits);
    BorderCredits->SetContent(BtnCredits);
    TabPanel->AddChild(BorderCredits);
    BtnCredits->OnClicked.AddUniqueDynamic(this, &UOptionsScreen::OnCreditsClicked);
    UE_LOG(LogOptionsScreen, Log, TEXT("[OptionsCredits] Credits tab and page ready"));
}

void UOptionsScreen::ReloadCredits()
{
    if (!CreditsText) return;
    FString Text;
    const TArray<FString> Paths = {
        FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/credits.txt")),
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/credits.txt"))
    };
    for (const FString& Path : Paths)
        if (FPaths::FileExists(Path) && FFileHelper::LoadFileToString(Text, *Path)) break;
    if (Text.IsEmpty())
    {
        Text = TEXT("STARSHATTER WARS\n\nFractal Dev Studios\n\nCredits unavailable: add credits.txt under Content/Data or Data.");
        UE_LOG(LogOptionsScreen, Warning, TEXT("[OptionsCredits] credits.txt not found"));
    }
    CreditsText->SetText(FText::FromString(Text));
    if (CreditsScroll) CreditsScroll->ScrollToStart();
}

void UOptionsScreen::ShowCreditsDlg()
{
    if (!CreditsPage) BuildCreditsPage();
    if (!CreditsPage) return;
    SetActiveTab(BtnCredits);
    SwitchToWidget(CreditsPage.Get());
    if (CreditsScroll) CreditsScroll->ScrollToStart();
    if (APlayerController* PC = GetOwningPlayer())
        if (BtnCredits) BtnCredits->SetUserFocus(PC);
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
