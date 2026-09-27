/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           MenuDlg.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Main Menu dialog (legacy MenuDlg) for Unreal UMG.

    CHANGE NOTE (OPTIONS HUB)
    =========================
    Audio/Video/Controls/Keyboard/Joystick/Mods are now managed exclusively by
    UOptionsScreen via a WidgetSwitcher. Therefore:

      - MenuDlg no longer binds or routes any "Video", "Controls", or "Mod" buttons.
      - MenuDlg exposes ONLY one "Options" entry point: ShowOptionsScreen().
      - Sub-option navigation is handled entirely within OptionsScreen.

=============================================================================*/

#include "MenuDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/TextBlock.h"

// Legacy
#include "Starshatter.h"
#include "Campaign.h"
#include "MenuScreen.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMenuDlg, Log, All);

UMenuDlg::UMenuDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMenuDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Static strings (only need to set once)
    AltStart = TEXT("Start a new game, or resume your current game");
    AltCampaign = TEXT("Start a new dynamic campaign, or load a saved game");
    AltMission = TEXT("Play or create a scripted mission exercise");
    AltMulti = TEXT("Start or join a multiplayer scenario");
    AltPlayer = TEXT("Manage your logbook and player preferences");

    // IMPORTANT: all sub-option routing is now inside OptionsScreen:
    AltOptions = TEXT("Audio, Video, Gameplay, Control, and Mod configuration options");

    AltTac = TEXT("View ship and weapon stats and mission roles");
    AltQuit = TEXT("Exit Starshatter and return to Windows");

    BindUMGDelegates();
}

void UMenuDlg::NativeConstruct()
{
    Super::NativeConstruct();

    // APPLY UI THEME (OPTION A: already loaded from Project Settings during BOOT)
    if (UGameInstance* GI = GetGameInstance())
    {
        if (auto* StyleSS = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
        {
            StyleSS->ApplyMenuButtonStyle(BtnStart);
            StyleSS->ApplyMenuButtonStyle(BtnCampaign);
            StyleSS->ApplyMenuButtonStyle(BtnMission);
            StyleSS->ApplyMenuButtonStyle(BtnPlayer);
            StyleSS->ApplyMenuButtonStyle(BtnMulti);
            StyleSS->ApplyMenuButtonStyle(BtnOptions);
            StyleSS->ApplyMenuButtonStyle(BtnTac);
            StyleSS->ApplyMenuButtonStyle(BtnQuit);
        }
    }
    // Cache legacy singletons (fine to do here)
    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();

    // Version text
    if (VersionText)
    {
        VersionText->SetText(FText::FromString(Game::VersionInfo));
    }
    else
    {
        UE_LOG(LogMenuDlg, Warning, TEXT("VersionText is not bound (BindWidgetOptional)."));
    }

    RefreshFromPlayerState();
    ApplyMenuGating();

    // First run routing AFTER gating
    if (bFirstRun_NoPlayerSave)
    {
        if (Manager)
        {
            Manager->ShowFirstTimeDlg();
        }
        else
        {
            UE_LOG(LogMenuDlg, Warning, TEXT("First run detected, but Manager is null."));
        }
    }

    Show();
}

void UMenuDlg::BindUMGDelegates()
{
    if (bDelegatesBound)
        return;

    // Click bindings:
    if (BtnStart)    BtnStart->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnStart);
    if (BtnCampaign) BtnCampaign->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnCampaign);
    if (BtnMission)  BtnMission->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnMission);
    if (BtnPlayer)   BtnPlayer->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnPlayer);
    if (BtnMulti)    BtnMulti->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnMultiplayer);

    // SINGLE entry point for all options:
    if (BtnOptions)  BtnOptions->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnOptions);

    if (BtnTac)      BtnTac->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnTacReference);
    if (BtnQuit)     BtnQuit->OnClicked.AddUniqueDynamic(this, &UMenuDlg::OnQuit);

    // Hover bindings:
    if (BtnStart) { BtnStart->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Start);       BtnStart->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Start); }
    if (BtnCampaign) { BtnCampaign->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Campaign); BtnCampaign->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Campaign); }
    if (BtnMission) { BtnMission->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Mission);   BtnMission->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Mission); }
    if (BtnPlayer) { BtnPlayer->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Player);    BtnPlayer->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Player); }
    if (BtnMulti) { BtnMulti->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Multi);      BtnMulti->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Multi); }
    if (BtnOptions) { BtnOptions->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Options);   BtnOptions->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Options); }
    if (BtnTac) { BtnTac->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Tac);          BtnTac->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Tac); }
    if (BtnQuit) { BtnQuit->OnHovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonEnter_Quit);        BtnQuit->OnUnhovered.AddUniqueDynamic(this, &UMenuDlg::OnButtonExit_Quit); }

    const bool bAny =
        (BtnStart || BtnCampaign || BtnMission || BtnPlayer || BtnMulti ||
            BtnOptions || BtnTac || BtnQuit);

    if (!bAny)
    {
        UE_LOG(LogMenuDlg, Error, TEXT("NO BUTTON WIDGETS ARE BOUND. Check BP widget names match C++ and are variables."));
    }

    bDelegatesBound = true;
}

void UMenuDlg::RefreshFromPlayerState()
{
    // Default assumptions
    bFirstRun_NoPlayerSave = false;
    bHasCampaignSelected = true;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            if (!PlayerSS->HasLoaded())
            {
                PlayerSS->LoadFromBoot();
            }

            const bool bHasPlayerSave = PlayerSS->HadExistingSaveOnLoad();
            bFirstRun_NoPlayerSave = !bHasPlayerSave;

            if (bHasPlayerSave)
            {
                const FS_PlayerGameInfo& Info = PlayerSS->GetPlayerInfo();
                bHasCampaignSelected = (Info.Campaign > 0) && (!Info.CampaignRowName.IsNone());
            }
            else
            {
                bHasCampaignSelected = false;
            }
        }
    }
}

void UMenuDlg::Show()
{
    // Legacy behavior: disable multiplayer when UseFileSystem() is true:
    if (BtnMulti && Starshatter::UseFileSystem())
        BtnMulti->SetIsEnabled(false);

    ClearDescription();
}

void UMenuDlg::ExecFrame(double DeltaTime)
{
    (void)DeltaTime;
}

// --------------------------------------------------------------------
// Click handlers
// --------------------------------------------------------------------

void UMenuDlg::OnStart()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnStart()"));

    if (Stars)
        Stars->StartOrResumeGame();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Stars is null in OnStart()."));
}

void UMenuDlg::OnCampaign()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnCampaign()"));

    if (Manager)
        Manager->ShowCampaignSelectDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnCampaign()."));
}

void UMenuDlg::OnMission()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnMission()"));

    if (Manager)
        Manager->ShowMissionSelectDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnMission()."));
}

void UMenuDlg::OnPlayer()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnPlayer()"));

    if (Manager)
        Manager->ShowPlayerDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnPlayer()."));
}

void UMenuDlg::OnMultiplayer()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnMultiplayer()"));
    // Hook this up when multiplayer lobby routing exists
}

void UMenuDlg::OnOptions()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnOptions()"));

    if (Manager)
        Manager->ShowOptionsScreen();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnOptions()."));
}

void UMenuDlg::OnTacReference()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnTacReference()"));

    if (Manager)
        Manager->ShowTacRefDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnTacReference()."));
}

void UMenuDlg::OnQuit()
{
    ClearDescription();
    UE_LOG(LogMenuDlg, Log, TEXT("OnQuit()"));

    if (Manager)
        Manager->ShowExitDlg();
    else
        UE_LOG(LogMenuDlg, Warning, TEXT("Manager is null in OnQuit()."));
}

// --------------------------------------------------------------------
// Hover handlers
// --------------------------------------------------------------------

void UMenuDlg::OnButtonEnter_Start() { SetDescription(AltStart); }
void UMenuDlg::OnButtonExit_Start() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Campaign() { SetDescription(AltCampaign); }
void UMenuDlg::OnButtonExit_Campaign() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Mission() { SetDescription(AltMission); }
void UMenuDlg::OnButtonExit_Mission() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Player() { SetDescription(AltPlayer); }
void UMenuDlg::OnButtonExit_Player() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Multi() { SetDescription(AltMulti); }
void UMenuDlg::OnButtonExit_Multi() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Options() { SetDescription(AltOptions); }
void UMenuDlg::OnButtonExit_Options() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Tac() { SetDescription(AltTac); }
void UMenuDlg::OnButtonExit_Tac() { ClearDescription(); }

void UMenuDlg::OnButtonEnter_Quit() { SetDescription(AltQuit); }
void UMenuDlg::OnButtonExit_Quit() { ClearDescription(); }

// --------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------

void UMenuDlg::ClearDescription()
{
    if (DescriptionText)
        DescriptionText->SetText(FText::GetEmpty());
}

void UMenuDlg::SetDescription(const FString& Text)
{
    if (DescriptionText)
        DescriptionText->SetText(FText::FromString(Text));
}

void UMenuDlg::SetButtonEnabled(UButton* Button, bool bEnable)
{
    if (Button)
        Button->SetIsEnabled(bEnable);
}

void UMenuDlg::EnableMenuButtons(bool bEnable)
{
    SetButtonEnabled(BtnStart, bEnable);
    SetButtonEnabled(BtnCampaign, bEnable);
    SetButtonEnabled(BtnMission, bEnable);
    SetButtonEnabled(BtnPlayer, bEnable);

    if (BtnMulti && Starshatter::UseFileSystem())
        SetButtonEnabled(BtnMulti, false);
    else
        SetButtonEnabled(BtnMulti, bEnable);

    // Options is a top-level menu entry now (single entry point to all sub-options):
    SetButtonEnabled(BtnOptions, bEnable);

    SetButtonEnabled(BtnTac, bEnable);

    // Quit is always allowed in your legacy logic
    SetButtonEnabled(BtnQuit, true);
}

/* --------------------------------------------------------------------
   ApplyMenuGating
   - MenuDlg gating is based on state, not "first run overlay"
   - First run overlay disabling is handled by MenuScreen
   -------------------------------------------------------------------- */
void UMenuDlg::ApplyMenuGating()
{
    // Default: enable everything (except multiplayer rule inside EnableMenuButtons)
    EnableMenuButtons(true);

    // NO CAMPAIGN SELECTED:
    // Enable: Campaign, Mission, Options, Tac, Quit
    // Disable: Start, Player, Multi
    if (!bHasCampaignSelected)
    {
        EnableMenuButtons(false);

        SetButtonEnabled(BtnPlayer, true);
        SetButtonEnabled(BtnCampaign, true);
        SetButtonEnabled(BtnMission, true);

        SetButtonEnabled(BtnOptions, true);

        SetButtonEnabled(BtnTac, true);
        SetButtonEnabled(BtnQuit, true);

        // Preserve legacy multiplayer filesystem rule
        if (BtnMulti && Starshatter::UseFileSystem())
            SetButtonEnabled(BtnMulti, false);
    }
}
