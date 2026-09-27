/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    DIALOG:      FirstTimeDlg
    FILE:        FirstTimeDlg.cpp
    AUTHOR:      Carlos Bott

    OVERVIEW
    ========
    First-time player setup dialog.

    Authoritative-only implementation:
      - Reads/writes FS_PlayerGameInfo via UStarshatterPlayerSubsystem
      - Saves via UStarshatterPlayerSubsystem::SavePlayer(true)
      - Does NOT touch legacy PlayerCharacter or KeyMap
*/

#include "FirstTimeDlg.h"

// UMG:
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"

// UE:
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// Subsystem:
#include "StarshatterPlayerSubsystem.h"

// Router:
#include "MenuScreen.h"
#include "MenuDlg.h"
// #include "PlayerDlg.h" // only if you want to directly refresh PlayerDlg here

DEFINE_LOG_CATEGORY_STATIC(LogFirstTimeDlg, Log, All);

UFirstTimeDlg::UFirstTimeDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UFirstTimeDlg::NativeConstruct()
{
    Super::NativeConstruct();

    PopulateDefaultsIfNeeded();

    if (AcceptBtn)
    {
        AcceptBtn->OnClicked.RemoveAll(this);
        AcceptBtn->OnClicked.AddDynamic(this, &UFirstTimeDlg::OnAcceptClicked);
    }
    else
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("AcceptBtn is NULL (BindWidget name mismatch / IsVariable off)."));
    }

    if (CancelBtn)
    {
        CancelBtn->OnClicked.RemoveAll(this);
        CancelBtn->OnClicked.AddDynamic(this, &UFirstTimeDlg::OnCancelClicked);
    }
}

UStarshatterPlayerSubsystem* UFirstTimeDlg::GetPlayerSubsystem() const
{
    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterPlayerSubsystem>();
}

void UFirstTimeDlg::PopulateDefaultsIfNeeded()
{
    // Play Style
    if (PlayStyleCombo)
    {
        if (PlayStyleCombo->GetOptionCount() == 0)
        {
            PlayStyleCombo->ClearOptions();
            PlayStyleCombo->AddOption(TEXT("Arcade Style"));
            PlayStyleCombo->AddOption(TEXT("Standard Model"));
        }

        if (PlayStyleCombo->GetSelectedIndex() < 0)
            PlayStyleCombo->SetSelectedIndex(0);
    }
    else
    {
        UE_LOG(LogFirstTimeDlg, Warning, TEXT("PlayStyleCombo is NULL (BindWidget mismatch)."));
    }

    // Experience
    if (ExperienceCombo)
    {
        if (ExperienceCombo->GetOptionCount() == 0)
        {
            ExperienceCombo->ClearOptions();
            ExperienceCombo->AddOption(TEXT("Cadet (First timer)"));
            ExperienceCombo->AddOption(TEXT("Admiral (Experienced)"));
        }

        if (ExperienceCombo->GetSelectedIndex() < 0)
            ExperienceCombo->SetSelectedIndex(0);
    }
    else
    {
        UE_LOG(LogFirstTimeDlg, Warning, TEXT("ExperienceCombo is NULL (BindWidget mismatch)."));
    }

    // Name hint
    if (NameEdit && NameEdit->GetHintText().IsEmpty())
        NameEdit->SetHintText(FText::FromString(TEXT("Player Name")));

    // Callsign hint
    if (CallsignEdit && CallsignEdit->GetHintText().IsEmpty())
        CallsignEdit->SetHintText(FText::FromString(TEXT("Player Callsign")));

    // Empire (only the 3 you want selectable)
    PopulateEmpireOptionsIfNeeded();
}

void UFirstTimeDlg::PopulateEmpireOptionsIfNeeded()
{
    if (!EmpireCombo)
    {
        UE_LOG(LogFirstTimeDlg, Warning, TEXT("EmpireCombo is NULL (BindWidget mismatch)."));
        return;
    }

    if (EmpireCombo->GetOptionCount() == 0)
    {
        EmpireCombo->ClearOptions();
        EmpireCombo->AddOption(TEXT("Terellian Alliance"));
        EmpireCombo->AddOption(TEXT("Marakan Hegemony"));
        EmpireCombo->AddOption(TEXT("Dantari Separatists"));
    }

    if (EmpireCombo->GetSelectedIndex() < 0)
        EmpireCombo->SetSelectedIndex(0);
}

EEMPIRE_NAME UFirstTimeDlg::EmpireFromComboIndex(int32 Index) const
{
    // Fixed order: 0 Terellian, 1 Marakan, 2 Dantari
    switch (Index)
    {
    default:
    case 0: return EEMPIRE_NAME::Terellian;
    case 1: return EEMPIRE_NAME::Marakan;
    case 2: return EEMPIRE_NAME::Dantari;
    }
}

void UFirstTimeDlg::EnsureMinimalFirstRunDefaults(FS_PlayerGameInfo& Info) const
{
    // If you want: guarantee non-empty identity fields on first save.
    // Keep it conservative; your save system can handle empty strings if you prefer.
    if (Info.Name.TrimStartAndEnd().IsEmpty())
        Info.Name = TEXT("NEW PILOT");

    if (Info.Callsign.TrimStartAndEnd().IsEmpty())
        Info.Callsign = TEXT("ROOKIE");
}

void UFirstTimeDlg::ApplyUiToPlayerInfo(FS_PlayerGameInfo& Info) const
{
    // -----------------------------
    // Identity
    // -----------------------------
    if (NameEdit)
    {
        const FString PlayerName = NameEdit->GetText().ToString().TrimStartAndEnd();
        if (!PlayerName.IsEmpty())
            Info.Name = PlayerName;
    }

    if (CallsignEdit)
    {
        const FString CS = CallsignEdit->GetText().ToString().TrimStartAndEnd();
        if (!CS.IsEmpty())
            Info.Callsign = CS;
    }

    // -----------------------------
    // Empire (authoritative int32)
    // -----------------------------
    if (EmpireCombo)
    {
        int32 Sel = EmpireCombo->GetSelectedIndex();
        if (Sel < 0) Sel = 0;

        const EEMPIRE_NAME EmpireEnum = EmpireFromComboIndex(Sel);
        Info.Empire = static_cast<int32>(EmpireEnum);
    }

    // -----------------------------
    // Play Style -> PlayerInfo options
    // -----------------------------
    int32 PlayStyleIndex = 0;
    if (PlayStyleCombo)
    {
        PlayStyleIndex = PlayStyleCombo->GetSelectedIndex();
        if (PlayStyleIndex < 0) PlayStyleIndex = 0;
    }

    if (PlayStyleIndex == 0)
    {
        // ARCADE
        Info.FlightModel = 2;
        Info.LandingMode = 1;
        Info.HudMode = 0;
        Info.GunSightMode = true;
    }
    else
    {
        // STANDARD
        Info.FlightModel = 0;
        Info.LandingMode = 0;
        Info.HudMode = 0;
        Info.GunSightMode = false;
    }

    // -----------------------------
    // Experience
    // -----------------------------
    int32 ExpIndex = 0;
    if (ExperienceCombo)
    {
        ExpIndex = ExperienceCombo->GetSelectedIndex();
        if (ExpIndex < 0) ExpIndex = 0;
    }

    if (ExpIndex > 0)
    {
        // Experienced
        Info.Rank = 2;                 // Lieutenant
        Info.HighestTrainingMission = 255;
        Info.Trained = 255;
        Info.TrainingMask = -1;        // optional: if your logic treats -1 as all bits set
    }
    else
    {
        // First timer (don’t overwrite if you already have something meaningful)
        if (Info.Rank < 0) Info.Rank = 0;
    }
}

void UFirstTimeDlg::OnCancelClicked()
{
    UE_LOG(LogFirstTimeDlg, Log, TEXT("UFirstTimeDlg::OnCancelClicked()"));

    if (MenuManager)
    {
        MenuManager->ShowMenuDlg();

        if (UMenuDlg* MenuDlg = MenuManager->GetMenuDlg())
        {
            MenuDlg->RefreshFromPlayerState();
            MenuDlg->ApplyMenuGating();
            MenuDlg->SetDialogInputEnabled(true);
        }
    }
}

void UFirstTimeDlg::OnAcceptClicked()
{
    UE_LOG(LogFirstTimeDlg, Warning, TEXT("UFirstTimeDlg::OnAcceptClicked()"));

    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem();
    if (!PlayerSS)
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("Player subsystem missing"));
        return;
    }

    // Make sure subsystem has loaded its authoritative info
    PlayerSS->LoadFromBoot();

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();

    // Apply UI -> Info
    ApplyUiToPlayerInfo(Info);

    // Optional: enforce minimal defaults on first save
    EnsureMinimalFirstRunDefaults(Info);

    PlayerSS->DebugDump(TEXT(" BEFORE ApplyUiToPlayerInfo"));
    ApplyUiToPlayerInfo(Info);
    EnsureMinimalFirstRunDefaults(Info);
    PlayerSS->DebugDump(TEXT(" BEFORE SavePlayer(true)"));
    const bool bOk = PlayerSS->SavePlayer(true);
    PlayerSS->DebugDump(TEXT(" AFTER SavePlayer(true)"));
    if (!bOk)
    {
        UE_LOG(LogFirstTimeDlg, Error, TEXT("SavePlayer(true) failed"));
        return;
    }

    // Router back to menu
    if (MenuManager)
    {
        MenuManager->ShowMenuDlg();

        // IMPORTANT: anything that displays player data should refresh FROM SUBSYSTEM/PlayerInfo now.
        if (UMenuDlg* MenuDlg = MenuManager->GetMenuDlg())
        {
            MenuDlg->RefreshFromPlayerState();
            MenuDlg->ApplyMenuGating();
            MenuDlg->SetDialogInputEnabled(true);
        }

        // Apply this EXACT pattern anywhere you show player identity (PlayerDlg, HUD, etc.)
        // if (UPlayerDlg* PlayerDlg = MenuManager->GetPlayerDlg())
        // {
        //     PlayerDlg->RefreshFromPlayerInfo();
        // }
    }
}
