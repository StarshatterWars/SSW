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
#include "AudioDlg.h"
#include "VideoDlg.h"
#include "ControlOptionsDlg.h"
#include "GameOptionsDlg.h"
#include "JoyDlg.h"
#include "KeyDlg.h"
#include "OptionsPage.h"
#include "StarshatterSettingsSaveSubsystem.h"
#include "Components/ScaleBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"


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

TSharedRef<SWidget> UOptionsScreen::RebuildWidget()
{
    // Reuse every authored page and control; only replace their outer presentation.
    BuildCreditsPage();
    BuildModernShell();
    return Super::RebuildWidget();
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
    if (auto* Page = Cast<UUserWidget>(Widget)) StylePage(Page);
}

// ------------------------------------------------------------
// Tab visuals
// ------------------------------------------------------------

void UOptionsScreen::StandardizeTopTabs()
{
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick, BtnKeyboard, BtnMods, BtnVideo, BtnCredits };
    const TCHAR* Labels[] = { TEXT("AUDIO"), TEXT("CONTROLS"), TEXT("GAME"), TEXT("JOYSTICK"), TEXT("KEYBOARD"), TEXT("MODS"), TEXT("VIDEO"), TEXT("CREDITS") };
    for (int32 Index=0; Index<UE_ARRAY_COUNT(Tabs); ++Index)
        if (UButton* Tab=Tabs[Index])
        {
            MissionUIStyle::ApplyActionButtonStyle(Tab, Index==ActiveTabIndex ? TEXT("Blue") : TEXT("Cyan"));
            Tab->SetIsEnabled(true);
            if (auto* Label=Cast<UTextBlock>(Tab->GetContent()))
            {
                Label->SetText(FText::FromString(Labels[Index]));
                Label->SetFont(MissionUIStyle::GetHeaderFont(22));
                Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
                Label->SetJustification(ETextJustify::Center);
            }
        }
    MissionUIStyle::ApplyActionButtonStyle(BtnCancel,TEXT("Red"));
    MissionUIStyle::ApplyActionButtonStyle(BtnSave,TEXT("Green"));
}

void UOptionsScreen::SetActiveTab(UButton* ActiveButton)
{
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick, BtnKeyboard, BtnMods, BtnVideo, BtnCredits };
    for (int32 Index=0; Index<UE_ARRAY_COUNT(Tabs); ++Index)
        if (Tabs[Index] && Tabs[Index]==ActiveButton) ActiveTabIndex=Index;
    StandardizeTopTabs();
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
    return ActiveTabIndex;
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

    if (auto* Page=Cast<UAudioDlg>(AudioDlg)) Page->Apply();
    if (auto* Page=Cast<UControlOptionsDlg>(ControlsDlg)) Page->Apply();
    if (auto* Page=Cast<UGameOptionsDlg>(GameDlg)) Page->Apply();
    if (auto* Page=Cast<UJoyDlg>(JoystickDlg)) Page->Apply();
    if (auto* Page=Cast<UKeyDlg>(KeyboardDlg)) Page->Apply();
    if (auto* Page=Cast<UVideoDlg>(VideoDlg)) Page->Apply();
    if (ModsDlg && ModsDlg->Implements<UOptionsPage>())
    {
        IOptionsPage::Execute_ApplySettings(ModsDlg);
        IOptionsPage::Execute_SaveSettings(ModsDlg);
    }
    auto* Save=GetGameInstance()?GetGameInstance()->GetSubsystem<UStarshatterSettingsSaveSubsystem>():nullptr;
    bool Saved=false;
    if (Save && (Save->HasSettings() || Save->LoadOrCreateSettings()))
    {
        Save->CaptureCurrentOptions();
        Saved=Save->SaveSettings();
    }
    if (OptionsSaveStatus) OptionsSaveStatus->SetText(FText::FromString(Saved?TEXT("Options saved"):TEXT("SAVE FAILED - PLEASE RETRY")));
    if (!Saved) UE_LOG(LogOptionsScreen,Error,TEXT("[Options] Could not save options.sav"));

}

void UOptionsScreen::CancelOptions()
{
    UE_LOG(LogOptionsScreen, Log, TEXT("[Options] CancelOptions()"));

    if (auto* Page=Cast<UAudioDlg>(AudioDlg)) Page->Cancel();
    if (auto* Page=Cast<UControlOptionsDlg>(ControlsDlg)) Page->Cancel();
    if (auto* Page=Cast<UGameOptionsDlg>(GameDlg)) Page->Cancel();
    if (auto* Page=Cast<UJoyDlg>(JoystickDlg)) Page->Cancel();
    if (auto* Page=Cast<UKeyDlg>(KeyboardDlg)) Page->Cancel();
    if (auto* Page=Cast<UVideoDlg>(VideoDlg)) Page->Cancel();
    if (ModsDlg && ModsDlg->Implements<UOptionsPage>()) IOptionsPage::Execute_CancelChanges(ModsDlg);

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

void UOptionsScreen::BuildModernShell()
{
    if (ModernShell || !WidgetTree || !OptionsSwitcher) return;
    UButton* Tabs[] = { BtnAudio, BtnControls, BtnGame, BtnJoystick, BtnKeyboard, BtnMods, BtnVideo, BtnCredits };
    // A partial Blueprint must retain its authored layout rather than lose navigation.
    for (UButton* Tab : Tabs) if (!Tab) return;
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("OptionsModernScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    auto* Frame=WidgetTree->ConstructWidget<USizeBox>(); Frame->SetWidthOverride(1920); Frame->SetHeightOverride(1080); Scale->SetContent(Frame);
    auto* Background=WidgetTree->ConstructWidget<UBorder>(); Background->SetBrushColor(FLinearColor(0.025f,0.055f,0.083f,1)); Background->SetPadding(FMargin(18)); Frame->SetContent(Background);
    auto* Column=WidgetTree->ConstructWidget<UVerticalBox>(); Background->SetContent(Column);
    auto* Header=WidgetTree->ConstructWidget<UBorder>(); Header->SetBrushColor(FLinearColor(0.105f,0.113f,0.125f,1)); Header->SetPadding(FMargin(18,9));
    if (!TitleText) TitleText=WidgetTree->ConstructWidget<UTextBlock>(); else TitleText->RemoveFromParent();
    TitleText->SetText(FText::FromString(TEXT("OPTIONS"))); TitleText->SetFont(MissionUIStyle::GetHeaderFont(38)); TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White)); auto* HeaderRow=WidgetTree->ConstructWidget<UHorizontalBox>();
    HeaderRow->AddChildToHorizontalBox(TitleText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    OptionsSaveStatus=WidgetTree->ConstructWidget<UTextBlock>();OptionsSaveStatus->SetFont(MissionUIStyle::GetRowFont(18));OptionsSaveStatus->SetColorAndOpacity(FSlateColor(FLinearColor(0.16f,0.77f,0.91f,1)));
    HeaderRow->AddChildToHorizontalBox(OptionsSaveStatus)->SetVerticalAlignment(VAlign_Center);Header->SetContent(HeaderRow);
    Column->AddChildToVerticalBox(Header)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    auto* Rule=WidgetTree->ConstructWidget<UBorder>(); Rule->SetBrushColor(FLinearColor(0.16f,0.77f,0.91f,1));
    auto* RuleSize=WidgetTree->ConstructWidget<USizeBox>(); RuleSize->SetHeightOverride(5); RuleSize->SetContent(Rule); Column->AddChildToVerticalBox(RuleSize);
    auto* Strip=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Strip)->SetPadding(FMargin(0,12));
    auto Arrow=[this,Strip](TObjectPtr<UButton>& Button,const TCHAR* Path,const TCHAR* Tip,const FLinearColor& Color)
    {
        if (!Button) Button=WidgetTree->ConstructWidget<UButton>(); else Button->RemoveFromParent();
        Button->SetVisibility(ESlateVisibility::Visible); Button->SetRenderOpacity(1.f);
        auto* Icon=WidgetTree->ConstructWidget<UImage>(); MissionUIStyle::SetActionIcon(Icon,Path,Color);
        auto* Box=WidgetTree->ConstructWidget<USizeBox>(); Box->SetWidthOverride(40);Box->SetHeightOverride(40);Box->SetContent(Icon);Button->SetContent(Box);
        if (auto* IconSlot=Cast<UButtonSlot>(Box->Slot)) { IconSlot->SetHorizontalAlignment(HAlign_Center);IconSlot->SetVerticalAlignment(VAlign_Center);IconSlot->SetPadding(FMargin(4)); }
        Button->SetToolTipText(FText::FromString(Tip));
        auto* Cell=Strip->AddChildToHorizontalBox(Button);Cell->SetPadding(FMargin(6,0));Cell->SetVerticalAlignment(VAlign_Fill);
    };
    Arrow(BtnCancel,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_L_256x256.T_Arrow_L_256x256"),TEXT("Back - discard unapplied changes"),FLinearColor(1,0.12f,0.08f,1));
    for (UButton* Tab : Tabs)
    {
        Tab->RemoveFromParent();
        auto* Caption=WidgetTree->ConstructWidget<UTextBlock>();Tab->SetContent(Caption);
        auto* Cell=Strip->AddChildToHorizontalBox(Tab);Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Cell->SetPadding(FMargin(5,0));
    }
    Arrow(BtnSave,TEXT("/Game/ProIconPack/Textures/Generic_Icons/256x256/T_Arrow_R_256x256.T_Arrow_R_256x256"),TEXT("Apply changes across all settings pages"),FLinearColor(0.1f,1,0.2f,1));
    OptionsSwitcher->RemoveFromParent();
    auto* Content=WidgetTree->ConstructWidget<UBorder>();Content->SetBrushColor(FLinearColor(0.015f,0.025f,0.038f,1));Content->SetPadding(FMargin(12));Content->SetContent(OptionsSwitcher);
    Column->AddChildToVerticalBox(Content)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    WidgetTree->RootWidget=Scale;ModernShell=Scale;
    // Bind any arrows created above; AddUnique preserves existing Blueprint delegates.
    BtnSave->OnClicked.AddUniqueDynamic(this,&UOptionsScreen::OnSaveClicked);
    BtnCancel->OnClicked.AddUniqueDynamic(this,&UOptionsScreen::OnCancelClicked);
}

void UOptionsScreen::StylePage(UUserWidget* Page)
{
    if (!Page || !Page->WidgetTree) return;
    Page->WidgetTree->ForEachWidget([this](UWidget* Widget)
    {
        const FString Name=Widget->GetName();
        if (auto* Text=Cast<UTextBlock>(Widget))
        {
            Text->SetFont(MissionUIStyle::GetRowFont(18));
            Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        }
        if (auto* Slider=Cast<USlider>(Widget))
        {
            Slider->SetSliderBarColor(FLinearColor(0.10f,0.20f,0.28f,1));
            Slider->SetSliderHandleColor(FLinearColor(0.16f,0.77f,0.91f,1));
        }
        if (auto* Button=Cast<UButton>(Widget))
        {
            MissionUIStyle::ApplyActionButtonStyle(Button,TEXT("Blue"));
            // Header owns shared actions. Keep page objects/delegates for legacy callers.
            if (Name==TEXT("ApplyBtn") || Name==TEXT("CancelBtn") || Name==TEXT("MenuButton") ||
                Name==TEXT("AudTabButton") || Name==TEXT("VidTabButton") || Name==TEXT("CtlTabButton") ||
                Name==TEXT("OptTabButton") || Name==TEXT("ModTabButton") || Name==TEXT("JoyTabButton") || Name==TEXT("KeyTabButton")) Button->SetVisibility(ESlateVisibility::Collapsed);
        }
    });
}
