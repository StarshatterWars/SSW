/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           OptionsScreen.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UOptionsScreen

    Central Options Hub for Starshatter Wars.

    Responsibilities:
      - Owns all options sub-screens (already placed in WBP)
      - Uses a WidgetSwitcher to swap sub-dialogs
      - Manages tab buttons (radio-style behavior + highlight)
      - Handles Apply / Cancel routing
      - ESC returns to MenuScreen
      - TAB / SHIFT+TAB cycles tabs

    NOTES
    =====
    - This class does NOT CreateWidget() subpages. Subpages are instances already
      placed inside the WBP WidgetSwitcher.
    - Focus: UButton does not support SetIsFocusable(). Instead:
        * UOptionsScreen is focusable (bIsFocusable = true)
        * We set UI input mode focus to a real widget (BtnAudio slate widget)
        * NativeOnKeyDown handles TAB/ESC cycling/routing

    TAB HIGHLIGHT (OPTION B)
    ========================
    - Add Borders in WBP wrapping each tab button.
    - Name them exactly:
        BorderAudio, BorderControls, BorderGame, BorderJoystick,
        BorderKeyboard, BorderMods, BorderVideo
    - Selected: steel gray (alpha 1)
    - Unselected: same RGB but alpha 0 (invisible)

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "OptionsScreen.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class UWidgetSwitcher;
class UWidget;
class UUserWidget;
class UMenuScreen;

UCLASS()
class STARSHATTERWARS_API UOptionsScreen : public UBaseScreen
{
    GENERATED_BODY()

public:
    UOptionsScreen(const FObjectInitializer& ObjectInitializer);

protected:
    // ------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    // Keyboard routing (TAB/SHIFT+TAB, ESC)
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:
    // BaseScreen ESC fallback:
    virtual void HandleCancel() override;

    // Public API used by subdialogs:
    void ApplyOptions();
    void CancelOptions();

    // KeyDlg returns here:
    void ReturnFromKeyDlg();

    // Legacy-compatible routing names:
    void ShowAudDlg();
    void ShowCtlDlg();
    void ShowGameDlg();
    void ShowJoyDlg();
    void ShowKeyDlg();
    void ShowModDlg();
    void ShowVidDlg();

protected:
    // ------------------------------------------------------------
    // Core container (must exist in WBP)
    // ------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidgetSwitcher> OptionsSwitcher = nullptr;

    // ------------------------------------------------------------
    // Tab Buttons (alphabetical)
    // ------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnAudio = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnControls = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnGame = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnJoystick = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnKeyboard = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnMods = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnVideo = nullptr;

    // Optional bottom buttons (if you still have them in WBP)
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnSave = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BtnCancel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText = nullptr;

    // ------------------------------------------------------------
    // Sub Widgets (instances placed in WBP switcher)
    // ------------------------------------------------------------
    // IMPORTANT: these must be the ACTUAL instances inside the WBP, not classes.
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> AudioDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> ControlsDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> GameDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> JoystickDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> KeyboardDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> ModsDlg = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UUserWidget> VideoDlg = nullptr;

    // ------------------------------------------------------------
    // Tab highlight borders (OPTION B)
    // ------------------------------------------------------------
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderAudio = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderControls = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderGame = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderJoystick = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderKeyboard = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderMods = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> BorderVideo = nullptr;

private:
    // ------------------------------------------------------------
    // Delegate binding
    // ------------------------------------------------------------
    void BindDelegates();
    bool bDelegatesBound = false;

    // ------------------------------------------------------------
    // Switcher helpers (uses existing children in WBP)
    // ------------------------------------------------------------
    void SwitchToWidget(UWidget* Widget);

    // ------------------------------------------------------------
    // Tab visuals
    // ------------------------------------------------------------
    void SetActiveTab(UButton* ActiveButton);

    // OPTION B: border highlight (steel gray when selected, alpha 0 when not)
    void UpdateTabBorders(UButton* ActiveButton);
    static void SetBorderSelected(UBorder* Border, bool bSelected);

    // ------------------------------------------------------------
    // TAB navigation
    // ------------------------------------------------------------
    int32 GetCurrentTabIndex() const;
    void FocusTabByDelta(int32 Delta);

private:
    // ------------------------------------------------------------
    // Button handlers
    // ------------------------------------------------------------
    UFUNCTION() void OnAudioClicked();
    UFUNCTION() void OnControlsClicked();
    UFUNCTION() void OnGameClicked();
    UFUNCTION() void OnJoystickClicked();
    UFUNCTION() void OnKeyboardClicked();
    UFUNCTION() void OnModsClicked();
    UFUNCTION() void OnVideoClicked();

    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnCancelClicked();
};
