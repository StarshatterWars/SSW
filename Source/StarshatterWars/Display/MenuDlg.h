/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           MenuDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Main Menu dialog (legacy MenuDlg) implementation for Unreal UMG.

    CHANGE NOTE (OPTIONS HUB)
    =========================
    Audio/Video/Controls/Keyboard/Joystick/Mods are now managed exclusively by
    UOptionsScreen via a WidgetSwitcher. Therefore:

      - MenuDlg no longer binds or routes any "Video", "Controls", or "Mod" buttons.
      - MenuDlg exposes ONLY one Options entry point: ShowOptionsScreen().
      - Sub-option navigation is handled entirely within OptionsScreen.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "MenuDlg.generated.h"

class UButton;
class UTextBlock;

// Legacy forward decls:
class Starshatter;
class Campaign;
class UMenuScreen;

UCLASS()
class STARSHATTERWARS_API UMenuDlg : public UBaseScreen
{
    GENERATED_BODY()

public:
    UMenuDlg(const FObjectInitializer& ObjectInitializer);

protected:
    // Bind once (preferred for delegates)
    virtual void NativeOnInitialized() override;

    // Use for per-show refresh / gating re-eval
    virtual void NativeConstruct() override;

public:
    virtual void ExecFrame(double DeltaTime) override;

    void RefreshFromPlayerState();
    void ApplyMenuGating();
    void EnableMenuButtons(bool bEnable);

    // Manager/router (set this when you create the widget)
    UPROPERTY(BlueprintReadWrite, Category = "Menu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UMenuScreen> Manager = nullptr;

protected:
    // ------------------------------------------------------------
    // UMG Widgets (names must match Blueprint widget names exactly)
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnStart;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnCampaign;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnMission;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnPlayer;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnMulti;

    // SINGLE entry point for all settings (OptionsScreen hub):
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnOptions;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnTac;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> BtnQuit;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DescriptionText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> VersionText;

protected:
    // ------------------------------------------------------------
    // Runtime state
    // ------------------------------------------------------------

    Starshatter* Stars = nullptr;
    Campaign* CampaignPtr = nullptr;

    bool bFirstRun_NoPlayerSave = false;
    bool bHasCampaignSelected = true;
    bool bDelegatesBound = false;

    // Hover “alt text”
    FString AltStart;
    FString AltCampaign;
    FString AltMission;
    FString AltMulti;
    FString AltPlayer;
    FString AltOptions;
    FString AltTac;
    FString AltQuit;

protected:
    // ------------------------------------------------------------
    // Binding + lifecycle helpers
    // ------------------------------------------------------------

    void BindUMGDelegates();

    void SetButtonEnabled(UButton* Button, bool bEnable);
    void Show();

    void ClearDescription();
    void SetDescription(const FString& Text);

protected:
    // ------------------------------------------------------------
    // Click handlers (MUST be UFUNCTION for AddDynamic/AddUniqueDynamic)
    // ------------------------------------------------------------

    UFUNCTION() void OnStart();
    UFUNCTION() void OnCampaign();
    UFUNCTION() void OnMission();
    UFUNCTION() void OnPlayer();
    UFUNCTION() void OnMultiplayer();

    // SINGLE entry point:
    UFUNCTION() void OnOptions();

    UFUNCTION() void OnTacReference();
    UFUNCTION() void OnQuit();

    // ------------------------------------------------------------
    // Hover handlers (MUST be UFUNCTION for AddDynamic/AddUniqueDynamic)
    // ------------------------------------------------------------

    UFUNCTION() void OnButtonEnter_Start();
    UFUNCTION() void OnButtonExit_Start();

    UFUNCTION() void OnButtonEnter_Campaign();
    UFUNCTION() void OnButtonExit_Campaign();

    UFUNCTION() void OnButtonEnter_Mission();
    UFUNCTION() void OnButtonExit_Mission();

    UFUNCTION() void OnButtonEnter_Player();
    UFUNCTION() void OnButtonExit_Player();

    UFUNCTION() void OnButtonEnter_Multi();
    UFUNCTION() void OnButtonExit_Multi();

    UFUNCTION() void OnButtonEnter_Options();
    UFUNCTION() void OnButtonExit_Options();

    UFUNCTION() void OnButtonEnter_Tac();
    UFUNCTION() void OnButtonExit_Tac();

    UFUNCTION() void OnButtonEnter_Quit();
    UFUNCTION() void OnButtonExit_Quit();
};
